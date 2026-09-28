
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

LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::VulkanShaderCompiler(VkDevice device)
	: m_device(device)
{
}

LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::~VulkanShaderCompiler() = default;

LuxonEngine::Rendering::ShaderProgram* LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& compileProperties, std::string& error)
{
	const auto baseOptions = CreateCompileOptions(compileProperties.folderPath);

	SPIRVShaderProgram* finalProgram;

	if (compileProperties.type == ShaderProgramType::Rasterization) {

		std::vector<ref<SPIRVShader>> shaders;

		if (compileProperties.rasterProperties.vertexMain != nullptr) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(compileProperties.rasterProperties.vertexMain);
			options.targetProfile = CharToString(("vs_" + compileProperties.model).c_str());

			auto vertexShader = CompileShaderStage(shaderCode, codeLength, options, Vulkan_Vertex, stageError);

			if (vertexShader == nullptr) {
				error = "Error in compiling Vertex Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(vertexShader);
		}

		if (compileProperties.rasterProperties.geometryMain != nullptr) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(compileProperties.rasterProperties.geometryMain);
			options.targetProfile = CharToString(("gs_" + compileProperties.model).c_str());

			auto geometryShader = CompileShaderStage(shaderCode, codeLength, options, Vulkan_Geometry, stageError);

			if (geometryShader == nullptr) {
				error = "Error in compiling Geometry Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(geometryShader);
		}

		if (compileProperties.rasterProperties.pixelMain != nullptr) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(compileProperties.rasterProperties.pixelMain);
			options.targetProfile = CharToString(("ps_" + compileProperties.model).c_str());

			auto pixelShader = CompileShaderStage(shaderCode, codeLength, options, Vulkan_Fragment, stageError);

			if (pixelShader == nullptr) {
				error = "Error in compiling Pixel Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(pixelShader);
		}

		finalProgram = new Rasterization::SPIRVRasterizationProgram(shaders, m_device);
	}

	else if (compileProperties.type == ShaderProgramType::RayTracing) {
		auto options = baseOptions;
		options.targetProfile = CharToString(("lib_" + compileProperties.model).c_str());
		options.defines.push_back(L"_VK_RAY_TRACING");

		if (compileProperties.rayTracingProperties.rayGen == nullptr)
			options.defines.push_back(L"_VK_RAY_TRACING_LOCAL");

		ComPtr<IDxcBlob> pshaderObjectData;

		if (m_compiler->Compile(shaderCode, codeLength, options, pshaderObjectData, error) == false) {
			return nullptr;
		}

		finalProgram = new RayTracing::SPIRVRayTracingProgram((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
	}

	else if (compileProperties.type == ShaderProgramType::Compute) {
		auto options = baseOptions;
		options.entryPoint = CharToString(compileProperties.computeProperties.computeMain);
		options.targetProfile = CharToString(("cs_" + compileProperties.model).c_str());

		ComPtr<IDxcBlob> pshaderObjectData;

		if (m_compiler->Compile(shaderCode, codeLength, options, pshaderObjectData, error) == false) {
			return nullptr;
		}

		finalProgram = new Compute::SPIRVComputeProgram((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
	}

	else if (compileProperties.type == ShaderProgramType::Mesh) {
		// Phase 1: compile the whole file as a library to find the entry function of each stage from its [shader("...")] attribute
		auto libraryOptions = baseOptions;
		libraryOptions.targetProfile = CharToString(("lib_" + compileProperties.model).c_str());

		ComPtr<IDxcBlob> pLibraryObjectData;

		if (m_compiler->Compile(shaderCode, codeLength, libraryOptions, pLibraryObjectData, error) == false) {
			return nullptr;
		}

		SpvReflectShaderModule libraryReflection;

		if (spvReflectCreateShaderModule(pLibraryObjectData->GetBufferSize(), pLibraryObjectData->GetBufferPointer(), &libraryReflection) != SPV_REFLECT_RESULT_SUCCESS) {
			error = "Unknown Error when Creating Reflection";
			return nullptr;
		}

		std::map<SpvReflectShaderStageFlagBits, std::string> foundStages;

		for (UInt32 i = 0; i < libraryReflection.entry_point_count; i++) {
			auto& entryPoint = libraryReflection.entry_points[i];
			SpvReflectShaderStageFlagBits stage = entryPoint.shader_stage;

			if (m_validMeshStages.find(stage) == m_validMeshStages.end()) {
				error = std::string("Invalid Stage Found: ") + SPIRVVariableReflection::ShaderStageToString(stage) + " stage is not supported in Mesh shaders";
				spvReflectDestroyShaderModule(&libraryReflection);
				return nullptr;
			}

			if (foundStages.find(stage) != foundStages.end()) {
				error = std::string("Multiple ") + SPIRVVariableReflection::ShaderStageToString(stage) + " Stage Found";
				spvReflectDestroyShaderModule(&libraryReflection);
				return nullptr;
			}

			foundStages[stage] = entryPoint.name;
		}

		spvReflectDestroyShaderModule(&libraryReflection);

		// Phase 2: compile each stage separately with its entry function
		std::vector<SPIRVShaderData> shaders;

		for (auto& [stage, entryPoint] : foundStages) {
			if (entryPoint.empty())
				continue;

			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(entryPoint.c_str());
			options.targetProfile = m_validMeshStages[stage];
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

		finalProgram = new MeshShading::SPIRVMeshProgram(shaders, m_device);
	}

	else {
		error = "Unknown Shader Type";
		return nullptr;
	}

	return finalProgram;
}

bool LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::Initialize()
{
	std::string error;
	m_compiler = std::make_unique<DXC::DXCCompiler>();

	if (m_compiler->Initialize(error) == false) {
		return false;
	}

	return true;
}

LuxonEngine::Rendering::DXC::DXCCompileOptions LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::CreateCompileOptions(const std::wstring& includeDir) const
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

ref<LuxonEngine::Rendering::Vulkan::SPIRVShader> LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, Vulkan_Shader_Type shaderType, std::string& error)
{
	ComPtr<IDxcBlob> pshaderObjectData;

	if (m_compiler->Compile(source, size, options, pshaderObjectData, error) == false) {
		return nullptr;
	}

	ref<SPIRVShader> shader = std::make_shared<SPIRVShader>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), shaderType, m_device, WStringToString(options.entryPoint));
	return shader;
}

LuxonEngine::Rendering::Vulkan::SPIRVShaderData LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::CompileShaderStageData(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error)
{
	SPIRVShaderData shaderData{};

	if (m_compiler->Compile(source, size, options, shaderData.byteCode, error) == false) {
		shaderData.byteCode = nullptr;
	}

	return shaderData;
}
