
#include "vulkan-pch.h"
#include "VulkanShaderRegistery.h"

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

QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::VulkanShaderRegistery(VkDevice device)
	: m_compileArguments(16), m_device(device)
{
	// Create compiler-related objects
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));

	m_utils->CreateDefaultIncludeHandler(&m_includeHandler);

	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));

	// -E for the entry point (eg. 'main')
	m_compileArguments[0] = (WCHAR*)L"-E";
	m_compileArguments[1] = (WCHAR*)L"-Main";

	// -T for the target profile (eg. 'ps_6_6')
	m_compileArguments[2] = (WCHAR*)L"-T";
	m_compileArguments[3] = (WCHAR*)L"target";

	m_compileArguments[4] = (WCHAR*)L"-I";
	m_compileArguments[5] = (WCHAR*)L"direction";

	m_compileArguments[6] = (WCHAR*)L"-D";
	m_compileArguments[7] = (WCHAR*)L"_VULKAN";

	m_compileArguments[8] = (WCHAR*)L"-spirv";
	m_compileArguments[9] = (WCHAR*)L"-fspv-target-env=vulkan1.3";
	m_compileArguments[10] = (WCHAR*)L"-O3";
	m_compileArguments[11] = (WCHAR*)L"-fvk-use-dx-layout";
	m_compileArguments[12] = (WCHAR*)L"-D";
	m_compileArguments[13] = (WCHAR*)L"_VK_RAY_TRACING";
	m_compileArguments[14] = (WCHAR*)L"-D";
	m_compileArguments[15] = (WCHAR*)L"_VK_RAY_TRACING_LOCAL";
}

QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::~VulkanShaderRegistery()
{
	m_dxcCompiler->Release();
	m_includeHandler->Release();
	m_utils->Release();
}

void QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT)
{
}

ref<QuantumEngine::Rendering::ShaderProgram> QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::CompileProgram(const std::wstring& fileName, std::string& error)
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

	DxcBuffer sourceBuffer{
		.Ptr = buffer.data(),
		.Size = buffer.size(),
		.Encoding = DXC_CP_ACP,
	};

	auto path = std::filesystem::path(fileName);
	std::wstring shaderDir = path.parent_path().c_str(); /* extract directory from fileName */;

	m_compileArguments[5] = (WCHAR*)shaderDir.c_str();

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

	ref<SPIRVShaderProgram> finalProgram;

	if (strcmp(programType, "Rasterization") == 0) {

		std::vector<ref<SPIRVShader>> shaders;

		if (properties.contains("vsMain")) {
			std::string stageError;
			std::wstring wMain = CharToString(properties["vsMain"].as_string().c_str());
			m_compileArguments[m_mainIndex] = (WCHAR*)(wMain.c_str());

			std::string target("vs_");
			target += properties["model"].as_string().c_str();
			std::wstring wTarget = CharToString(target.c_str());
			m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());

			auto vertexShader = CompileShaderStage(&sourceBuffer, Vulkan_Vertex, WStringToString(wMain), stageError);

			if (vertexShader == nullptr) {
				error = "Error in compiling Vertex Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(vertexShader);
		}

		if (properties.contains("gsMain")) {
			std::string stageError;
			std::wstring wMain = CharToString(properties["gsMain"].as_string().c_str());
			m_compileArguments[m_mainIndex] = (WCHAR*)(wMain.c_str());

			std::string target("gs_");
			target += properties["model"].as_string().c_str();
			std::wstring wTarget = CharToString(target.c_str());
			m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());

			auto geometryShader = CompileShaderStage(&sourceBuffer, Vulkan_Geometry, WStringToString(wMain), stageError);

			if (geometryShader == nullptr) {
				error = "Error in compiling Geometry Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(geometryShader);
		}

		if (properties.contains("psMain")) {
			std::string stageError;
			std::wstring wMain = CharToString(properties["psMain"].as_string().c_str());
			m_compileArguments[m_mainIndex] = (WCHAR*)(wMain.c_str());

			std::string target("ps_");
			target += properties["model"].as_string().c_str();
			std::wstring wTarget = CharToString(target.c_str());
			m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());

			auto pixelShader = CompileShaderStage(&sourceBuffer, Vulkan_Fragment, WStringToString(wMain), stageError);

			if (pixelShader == nullptr) {
				error = "Error in compiling Pixel Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(pixelShader);
		}

		finalProgram = std::make_shared<Rasterization::SPIRVRasterizationProgram>(shaders, m_device);
	}

	else if(strcmp(programType, "RayTracing") == 0) {
		m_compileArguments[m_mainIndex] = (WCHAR*)L"";
		std::string target("lib_");
		target += properties["model"].as_string().c_str();
		std::wstring wTarget = CharToString(target.c_str());
		m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());

		ComPtr<IDxcBlob> pshaderObjectData;

		ComPtr<IDxcResult> compileResult;
		HRESULT result;
		UInt32 argumentCounts = properties.contains("rayGeneration") ? m_minArguments + 2 : m_minArguments + 4;
		result = m_dxcCompiler->Compile(&sourceBuffer, (LPCWSTR*)m_compileArguments.data(), argumentCounts, m_includeHandler, IID_PPV_ARGS(&compileResult));

		if (FAILED(result)) {
			error = "Unknown Error when Beginning to compile";
			return nullptr;
		}

		ComPtr<IDxcBlobUtf8> pErrors;

		result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

		if (FAILED(result)) {
			error = "Unknown Error when Beginning to compile";
			return nullptr;
		}

		if (pErrors && pErrors->GetStringLength() > 0)
		{
			error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
			return nullptr;
		}

		result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pshaderObjectData), nullptr);

		if (FAILED(result)) {
			error = "Unknown Error when Obtaining Shader Bytecode";
			return nullptr;
		}

		finalProgram = std::make_shared<RayTracing::SPIRVRayTracingProgram>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
	}

	else if (strcmp(programType, "Compute") == 0) {
		std::wstring wMain = CharToString(properties["csMain"].as_string().c_str());
		m_compileArguments[m_mainIndex] = (WCHAR*)(wMain.c_str());

		std::string target("cs_");
		target += properties["model"].as_string().c_str();
		std::wstring wTarget = CharToString(target.c_str());
		m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());

		ComPtr<IDxcBlob> pshaderObjectData;

		ComPtr<IDxcResult> compileResult;
		HRESULT result;
		result = m_dxcCompiler->Compile(&sourceBuffer, (LPCWSTR*)m_compileArguments.data(), m_minArguments, m_includeHandler, IID_PPV_ARGS(&compileResult));

		if (FAILED(result)) {
			error = "Unknown Error when Beginning to compile";
			return nullptr;
		}

		ComPtr<IDxcBlobUtf8> pErrors;

		result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

		if (FAILED(result)) {
			error = "Unknown Error when Beginning to compile";
			return nullptr;
		}

		if (pErrors && pErrors->GetStringLength() > 0)
		{
			error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
			return nullptr;
		}

		result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pshaderObjectData), nullptr);

		if (FAILED(result)) {
			error = "Unknown Error when Obtaining Shader Bytecode";
			return nullptr;
		}

		finalProgram = std::make_shared<Compute::SPIRVComputeProgram>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), m_device);
	}

	auto uidStr = metaData["uuid"].as_string().c_str();
	boost::uuids::string_generator gen;
	m_registeredPrograms.emplace(gen(uidStr), finalProgram);
	return finalProgram;
}

ref<QuantumEngine::Rendering::Vulkan::SPIRVShaderProgram> QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::GetShaderPrograms(const std::string& name)
{
	auto it = m_specialPrograms.find(name);
	if (it != m_specialPrograms.end())
		return (*it).second;
	return nullptr;
}

void QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::Initialize()
{
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
}

ref<QuantumEngine::Rendering::Vulkan::SPIRVShader> QuantumEngine::Rendering::Vulkan::VulkanShaderRegistery::CompileShaderStage(const DxcBuffer* sourceBuffer, Vulkan_Shader_Type shaderType, const std::string entryName, std::string& error)
{
	ComPtr<IDxcBlob> pshaderObjectData;

	ComPtr<IDxcResult> compileResult;
	HRESULT result;
	result = m_dxcCompiler->Compile(sourceBuffer, (LPCWSTR*)m_compileArguments.data(), m_minArguments, m_includeHandler, IID_PPV_ARGS(&compileResult));

	if (FAILED(result)) {
		error = "Unknown Error when Beginning to compile";
		return nullptr;
	}

	ComPtr<IDxcBlobUtf8> pErrors;

	result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

	if (FAILED(result)) {
		error = "Unknown Error when Beginning to compile";
		return nullptr;
	}

	if (pErrors && pErrors->GetStringLength() > 0)
	{
		error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
		return nullptr;
	}

	result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pshaderObjectData), nullptr);

	if (FAILED(result)) {
		error = "Unknown Error when Obtaining Shader Bytecode";
		return nullptr;
	}

	ref<SPIRVShader> shader = std::make_shared<SPIRVShader>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), shaderType, m_device, entryName);
	return shader;
}
