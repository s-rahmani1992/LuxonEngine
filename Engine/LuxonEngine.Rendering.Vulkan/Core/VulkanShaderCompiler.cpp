
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

void LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT)
{
}

ref<LuxonEngine::Rendering::ShaderProgram> LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::CompileProgram(const std::wstring& fileName, std::string& error)
{
	// Read file into memory
	std::ifstream shaderFile(fileName, std::ios::binary | std::ios::ate);
	if (!shaderFile) {
		error = "Failed to compile file at " + WStringToString(fileName) + "\nFailed to open shader file.";
		return nullptr;
	}

	std::streamsize size = shaderFile.tellg();
	shaderFile.seekg(0, std::ios::beg);
	std::vector<char> buffer(size);
	if (!shaderFile.read(buffer.data(), size)) {
		error = "Failed to read shader file.";
		return nullptr;
	}

	auto path = std::filesystem::path(fileName);
	std::wstring shaderDir = path.parent_path().c_str(); /* extract directory from fileName */;

	const auto baseOptions = CreateCompileOptions(shaderDir);

	std::ifstream metafile(WStringToString(fileName) + ".json", std::ios::in | std::ios::binary);
	if (!metafile) throw std::runtime_error("Failed to open meta file");

	auto jsonMetaStr = std::string(
		std::istreambuf_iterator<char>(metafile),
		std::istreambuf_iterator<char>()
	);

	boost::system::error_code ec;
	boost::json::value jv = boost::json::parse(jsonMetaStr, ec);
	auto& metaData = jv.as_object();
	auto& properties = metaData["data"].as_object();

	auto programType = properties["type"].as_string().c_str();
	std::string model = properties["model"].as_string().c_str();

	ref<SPIRVShaderProgram> finalProgram;

	if (strcmp(programType, "Rasterization") == 0) {

		std::vector<ref<SPIRVShader>> shaders;

		if (properties.contains("vsMain")) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(properties["vsMain"].as_string().c_str());
			options.targetProfile = CharToString(("vs_" + model).c_str());

			auto vertexShader = CompileShaderStage(buffer.data(), buffer.size(), options, Vulkan_Vertex, stageError);

			if (vertexShader == nullptr) {
				error = "Error in compiling Vertex Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(vertexShader);
		}

		if (properties.contains("gsMain")) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(properties["gsMain"].as_string().c_str());
			options.targetProfile = CharToString(("gs_" + model).c_str());

			auto geometryShader = CompileShaderStage(buffer.data(), buffer.size(), options, Vulkan_Geometry, stageError);

			if (geometryShader == nullptr) {
				error = "Error in compiling Geometry Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(geometryShader);
		}

		if (properties.contains("psMain")) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(properties["psMain"].as_string().c_str());
			options.targetProfile = CharToString(("ps_" + model).c_str());

			auto pixelShader = CompileShaderStage(buffer.data(), buffer.size(), options, Vulkan_Fragment, stageError);

			if (pixelShader == nullptr) {
				error = "Error in compiling Pixel Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(pixelShader);
		}

		finalProgram = std::make_shared<Rasterization::SPIRVRasterizationProgram>(shaders, m_device);
	}

	else if(strcmp(programType, "RayTracing") == 0) {
		auto options = baseOptions;
		options.targetProfile = CharToString(("lib_" + model).c_str());
		options.defines.push_back(L"_VK_RAY_TRACING");

		if (!properties.contains("rayGen"))
			options.defines.push_back(L"_VK_RAY_TRACING_LOCAL");

		ComPtr<IDxcBlob> pshaderObjectData;

		if (m_compiler->Compile(buffer.data(), buffer.size(), options, pshaderObjectData, error) == false) {
			return nullptr;
		}

		finalProgram = std::make_shared<RayTracing::SPIRVRayTracingProgram>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
	}

	else if (strcmp(programType, "Compute") == 0) {
		auto options = baseOptions;
		options.entryPoint = CharToString(properties["csMain"].as_string().c_str());
		options.targetProfile = CharToString(("cs_" + model).c_str());

		ComPtr<IDxcBlob> pshaderObjectData;

		if (m_compiler->Compile(buffer.data(), buffer.size(), options, pshaderObjectData, error) == false) {
			return nullptr;
		}

		finalProgram = std::make_shared<Compute::SPIRVComputeProgram>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
	}

	auto uidStr = metaData["uuid"].as_string().c_str();
	boost::uuids::string_generator gen;
	m_registeredPrograms.emplace(gen(uidStr), finalProgram);
	return finalProgram;
}

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

ref<LuxonEngine::Rendering::ShaderProgram> LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::GetProgramByGUID(boost::uuids::uuid guid)
{
	auto it = m_registeredPrograms.find(guid);

	if (it != m_registeredPrograms.end())
		return it->second;

	return nullptr;
}

ref<LuxonEngine::Rendering::Vulkan::SPIRVShaderProgram> LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::GetShaderPrograms(const std::string& name)
{
	auto it = m_specialPrograms.find(name);
	if (it != m_specialPrograms.end())
		return (*it).second;
	return nullptr;
}

bool LuxonEngine::Rendering::Vulkan::VulkanShaderCompiler::Initialize()
{
	std::string error;
	m_compiler = std::make_unique<DXC::DXCCompiler>();

	if (m_compiler->Initialize(error) == false) {
		return false;
	}

	std::wstring root = Platform::Application::GetExecutablePath();

	std::string errorStr;

	auto gBufferProgram = CompileProgram(root + L"\\Assets\\Shaders\\g_buffer_raster.hlsl", errorStr);

	if (gBufferProgram != nullptr) {
		m_specialPrograms.emplace("G_Buffer_Program", std::dynamic_pointer_cast<SPIRVShaderProgram>(gBufferProgram));
	}

	auto gBufferGlobalRTProgram = CompileProgram(root + L"\\Assets\\Shaders\\g_buffer_rt_global.lib.hlsl", errorStr);

	if (gBufferGlobalRTProgram != nullptr) {
		m_specialPrograms.emplace("G_Buffer_RT_Global_Program", std::dynamic_pointer_cast<SPIRVShaderProgram>(gBufferGlobalRTProgram));
	}

	auto computeProgram = CompileProgram(root + L"\\Assets\\Shaders\\curve_mesh_compute.cs.hlsl", errorStr);

	if (computeProgram != nullptr) {
		m_specialPrograms.emplace("Bezier_Curve_Compute_Program", std::dynamic_pointer_cast<SPIRVShaderProgram>(computeProgram));
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
