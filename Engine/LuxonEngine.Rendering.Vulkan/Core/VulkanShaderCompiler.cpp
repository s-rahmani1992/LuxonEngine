
#include "vulkan-pch.h"
#include "VulkanShaderCompiler.h"
#include <dxcapi.h>
#include "DXCCompiler.h"

#include <fstream>
#include <filesystem>

#include "SPIRVShader.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "RayTracing/SPIRVRayTracingProgram.h"
#include "Compute/SPIRVComputeProgram.h"
#include "Mesh/SPIRVMeshProgram.h"

#include <boost/uuid/string_generator.hpp>
#include <boost/json.hpp>

#include "StringUtilities.h"
#include "Platform/Application.h"

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::Vulkan
{
	const std::map<ShaderProgramType, std::map<SpvReflectShaderStageFlagBits, std::wstring>> s_validStages = {
		{ ShaderProgramType::Rasterization, {
			{ SPV_REFLECT_SHADER_STAGE_VERTEX_BIT, L"vs_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT, L"gs_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT, L"ps_6_6" },
		}},
		{ ShaderProgramType::Compute, {
			{ SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT, L"cs_6_6" },
		}},
		{ ShaderProgramType::RayTracing, {
			{ SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR, L"lib_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR, L"lib_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, L"lib_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR, L"lib_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR, L"lib_6_6" },
		}},
		{ ShaderProgramType::Mesh, {
			{ SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT, L"ps_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT, L"as_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT, L"ms_6_6" },
		}},
	};

	VulkanShaderCompiler::VulkanShaderCompiler(VkDevice device)
		: m_device(device)
	{
	}

	VulkanShaderCompiler::~VulkanShaderCompiler() = default;

	ShaderProgram* VulkanShaderCompiler::CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& compileProperties, std::string& error)
	{
		if (s_validStages.find(compileProperties.type) == s_validStages.end()) {
			error = "Unknown Shader Type";
			return nullptr;
		}

		const auto baseOptions = CreateCompileOptions(compileProperties.folderPath);

		auto discoveryOptions = baseOptions;

		//if (compileProperties.type == ShaderProgramType::RayTracing)
		//	discoveryOptions.defines.push_back(L"_VK_RAY_TRACING");

		std::map<SpvReflectShaderStageFlagBits, std::string> foundStages;

		if (DiscoverStages(shaderCode, codeLength, discoveryOptions, compileProperties.type, foundStages, error) == false) {
			return nullptr;
		}

		SPIRVShaderProgram* finalProgram;

		if (compileProperties.type == ShaderProgramType::Rasterization) {
			std::vector<ref<SPIRVShader>> shaders;

			for (auto& [stage, entryPoint] : foundStages) {
				Vulkan_Shader_Type shaderType = Vulkan_Fragment;

				if (stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
					shaderType = Vulkan_Vertex;
				else if (stage == SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT)
					shaderType = Vulkan_Geometry;

				std::string stageError;
				auto options = baseOptions;
				options.entryPoint = CharToString(entryPoint.c_str());
				options.targetProfile = s_validStages.at(ShaderProgramType::Rasterization).at(stage);

				auto shader = CompileShaderStage(shaderCode, codeLength, options, shaderType, stageError);

				if (shader == nullptr) {
					error = std::string("Error in compiling ") + SPIRVVariableReflection::ShaderStageToString(stage) + " Stage: " + stageError;
					return nullptr;
				}

				shaders.push_back(shader);
			}

			finalProgram = new Rasterization::SPIRVRasterizationProgram(shaders, m_device);
		}

		else if (compileProperties.type == ShaderProgramType::RayTracing) {
			auto options = baseOptions;
			options.targetProfile = CharToString("lib_6_6");
			options.defines.push_back(L"_VK_RAY_TRACING");

			if (foundStages.find(SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR) == foundStages.end())
				options.defines.push_back(L"_VK_RAY_TRACING_LOCAL");

			ComPtr<IDxcBlob> pshaderObjectData;

			if (m_compiler->Compile(shaderCode, codeLength, options, pshaderObjectData, error) == false) {
				return nullptr;
			}

			finalProgram = new RayTracing::SPIRVRayTracingProgram((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
		}

		else if (compileProperties.type == ShaderProgramType::Compute) {
			const auto& [stage, entryPoint] = *foundStages.begin();

			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(entryPoint.c_str());
			options.targetProfile = s_validStages.at(ShaderProgramType::Compute).at(stage);

			auto spirvShader = CompileShaderStageData(shaderCode, codeLength, options, stageError);

			if (spirvShader.byteCode == nullptr) {
				error = std::string("Error in compiling ") + SPIRVVariableReflection::ShaderStageToString(stage) + " Stage: " + stageError;
				return nullptr;
			}

			finalProgram = new Compute::SPIRVComputeProgram((Byte*)spirvShader.byteCode->GetBufferPointer(), spirvShader.byteCode->GetBufferSize(), m_device);
		}

		else {
			std::vector<SPIRVShaderData> shaders;

			for (auto& [stage, entryPoint] : foundStages) {
				std::string stageError;
				auto options = baseOptions;
				options.entryPoint = CharToString(entryPoint.c_str());
				options.targetProfile = s_validStages.at(ShaderProgramType::Mesh).at(stage);
				auto spirvShader = CompileShaderStageData(shaderCode, codeLength, options, stageError);

				if (spirvShader.byteCode == nullptr) {
					error = std::string("Error in compiling ") + SPIRVVariableReflection::ShaderStageToString(stage) + " Stage: " + stageError;
					return nullptr;
				}

				// the SPIR-V reflection stage bits have the same values as the Vulkan stage bits
				spirvShader.shaderType = (VkShaderStageFlagBits)stage;
				spirvShader.entryPoint = entryPoint;
				shaders.push_back(spirvShader);
			}

			auto meshProgram = new MeshShading::SPIRVMeshProgram(shaders, m_device);
			std::string layoutError;

			if (meshProgram->InitializePipelineLayout(layoutError) == false) {
				error = "Error in Creating Pipeline Layout: " + layoutError;
				delete meshProgram;
				return nullptr;
			}

			finalProgram = meshProgram;
		}

		return finalProgram;
	}

	bool VulkanShaderCompiler::Initialize()
	{
		std::string error;
		m_compiler = std::make_unique<DXC::DXCCompiler>();

		if (m_compiler->Initialize(error) == false) {
			return false;
		}

		return true;
	}

	DXC::DXCCompileOptions VulkanShaderCompiler::CreateCompileOptions(const std::wstring& includeDir) const
	{
		DXC::DXCCompileOptions options;
		options.includeDirs.push_back(includeDir);
		options.defines.push_back(L"_VULKAN");

		options.arguments = {
			L"-spirv",
			L"-fspv-target-env=vulkan1.3",
			L"-O3",
			L"-fvk-use-dx-layout",
		};

		return options;
	}

	bool VulkanShaderCompiler::DiscoverStages(const void* source, size_t size, const DXC::DXCCompileOptions& baseOptions, const ShaderProgramType shaderType, std::map<SpvReflectShaderStageFlagBits, std::string>& outStages, std::string& error)
	{
		auto libraryOptions = baseOptions;
		libraryOptions.targetProfile = CharToString("lib_6_6");

		ComPtr<IDxcBlob> pLibraryObjectData;

		if (m_compiler->Compile(source, size, libraryOptions, pLibraryObjectData, error) == false) {
			return false;
		}

		SpvReflectShaderModule libraryReflection;

		if (spvReflectCreateShaderModule(pLibraryObjectData->GetBufferSize(), pLibraryObjectData->GetBufferPointer(), &libraryReflection) != SPV_REFLECT_RESULT_SUCCESS) {
			error = "Unknown Error when Creating Reflection";
			return false;
		}

		auto& validStages = s_validStages.at(shaderType);
		bool success = true;

		for (UInt32 i = 0; i < libraryReflection.entry_point_count && success; i++) {
			auto& entryPoint = libraryReflection.entry_points[i];
			SpvReflectShaderStageFlagBits stage = entryPoint.shader_stage;

			if (validStages.find(stage) == validStages.end()) {
				error = std::string("Invalid Stage Found: ") + SPIRVVariableReflection::ShaderStageToString(stage) + " stage is not supported in this shader type";
				success = false;
			}
			else if (outStages.find(stage) != outStages.end()) {
				error = std::string("Multiple ") + SPIRVVariableReflection::ShaderStageToString(stage) + " Stage Found";
				success = false;
			}
			else {
				outStages[stage] = entryPoint.name;
			}
		}

		spvReflectDestroyShaderModule(&libraryReflection);

		if (success && outStages.empty()) {
			error = "No shader stage found. Entry points must be marked with the [shader(\"...\")] attribute";
			return false;
		}

		return success;
	}

	ref<SPIRVShader> VulkanShaderCompiler::CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, Vulkan_Shader_Type shaderType, std::string& error)
	{
		ComPtr<IDxcBlob> pshaderObjectData;

		if (m_compiler->Compile(source, size, options, pshaderObjectData, error) == false) {
			return nullptr;
		}

		ref<SPIRVShader> shader = std::make_shared<SPIRVShader>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), shaderType, m_device, WStringToString(options.entryPoint));
		return shader;
	}

	SPIRVShaderData VulkanShaderCompiler::CompileShaderStageData(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error)
	{
		SPIRVShaderData shaderData{};

		if (m_compiler->Compile(source, size, options, shaderData.byteCode, error) == false) {
			shaderData.byteCode = nullptr;
		}

		return shaderData;
	}
}
