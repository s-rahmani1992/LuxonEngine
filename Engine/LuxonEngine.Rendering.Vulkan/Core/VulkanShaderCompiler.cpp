
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
