#include "pch.h"
#include "DX12ShaderRegistery.h"

#include <fstream>
#include <filesystem>

#include "Rasterization/HLSLRasterizationProgram.h"
#include "RayTracing/HLSLRayTracingProgram.h"
#include "Compute/HLSLComputeProgram.h"

#include "StringUtilities.h"
#include <vector>
#include <memory>
#include <Platform/Application.h>
#include <dxcapi.h>

#include <boost/uuid/string_generator.hpp>
#include <boost/json.hpp>

#define RASTERIZATION "Rasterization"
#define RAY_TRACING "RayTracing"
#define COMPUTE "Compute"

namespace Render = QuantumEngine::Rendering;
namespace HLSL = QuantumEngine::Rendering::DX12::Rasterization;
namespace Compute = QuantumEngine::Rendering::DX12::Compute;

QuantumEngine::Rendering::DX12::DX12ShaderRegistery::DX12ShaderRegistery()
	:m_minArguments(10), m_compileArguments(15)
{
	// Create compiler-related objects
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));

	m_utils->CreateDefaultIncludeHandler(&m_includeHandler);

	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));

	m_compileArguments.reserve(20);
	// -E for the entry point (eg. 'main')
	m_compileArguments[0] = (WCHAR*)L"-E";
	m_compileArguments[1] = (WCHAR*)L"-Main";

	// -T for the target profile (eg. 'ps_6_6')
	m_compileArguments[2] = (WCHAR*)L"-T";
	m_compileArguments[3] = (WCHAR*)L"target";

	m_compileArguments[4] = (WCHAR*)L"-I";
	m_compileArguments[5] = (WCHAR*)L"direction";

	// Strip reflection data and pdbs (see later)
	m_compileArguments[6] = (WCHAR*)L"-Qstrip_debug";
	m_compileArguments[7] = (WCHAR*)L"-Qstrip_reflect";
	m_compileArguments[8] = (WCHAR*)DXC_ARG_WARNINGS_ARE_ERRORS; //-WX
	m_compileArguments[9] = (WCHAR*)DXC_ARG_DEBUG; //-Zi
	m_compileArguments[10] = (WCHAR*)L"-D";
	m_compileArguments[11] = (WCHAR*)L"";
}

QuantumEngine::Rendering::DX12::DX12ShaderRegistery::~DX12ShaderRegistery()
{
	m_dxcCompiler->Release();
	m_includeHandler->Release();
	m_utils->Release();
}

void QuantumEngine::Rendering::DX12::DX12ShaderRegistery::Initialize(const ComPtr<ID3D12Device10>& device)
{
	m_device = device;
	std::wstring root = Platform::Application::GetExecutablePath();

	std::string errorStr;

	auto gBufferProgram = CompileProgram(root + L"\\Assets\\Shaders\\g_buffer_raster.hlsl", errorStr);

	if(gBufferProgram != nullptr)
		m_specialShaders.emplace("G_Buffer_Program", std::dynamic_pointer_cast<HLSLShaderProgram>(gBufferProgram));
	
	auto reflectionRTLightProgram = CompileProgram(root + L"\\Assets\\Shaders\\g_buffer_rt_global.lib.hlsl", errorStr);

	if (reflectionRTLightProgram != nullptr)
		m_specialShaders.emplace("G_Buffer_RT_Global_Program", std::dynamic_pointer_cast<HLSLShaderProgram>(reflectionRTLightProgram));

	auto computeProgram = CompileProgram(root + L"\\Assets\\Shaders\\curve_mesh_compute.cs.hlsl", errorStr);

	if (computeProgram != nullptr) {
		m_specialShaders.emplace("Bezier_Curve_Compute_Program", std::dynamic_pointer_cast<HLSLShaderProgram>(computeProgram));
	}
}

ref<QuantumEngine::Rendering::DX12::HLSLShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::GetShaderProgram(const std::string& name)
{
	auto it = m_specialShaders.find(name);
	if (it != m_specialShaders.end())
		return (*it).second;
	return nullptr;
}

void QuantumEngine::Rendering::DX12::DX12ShaderRegistery::RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT)
{
	ref<HLSLShaderProgram> hlslProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(program);

	if(hlslProgram != nullptr) {
		m_specialShaders.emplace(name, hlslProgram);
	}
}

ref<QuantumEngine::Rendering::ShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::CompileProgram(const std::wstring& hlslFile, std::string& error)
{
	// Read file into memory
	std::ifstream shaderFile(hlslFile, std::ios::binary | std::ios::ate);
	if (!shaderFile) {
		error = "Failed to compile file at " + WStringToString(hlslFile) + "\nFailed to open shader file.";
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

	auto path = std::filesystem::path(hlslFile);
	std::wstring shaderDir = path.parent_path().c_str(); /* extract directory from fileName */;

	m_compileArguments[5] = (WCHAR*)shaderDir.c_str();

	std::ifstream metafile(WStringToString(hlslFile) + ".json", std::ios::in | std::ios::binary);
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
	
	ref<HLSLShaderProgram> finalProgram;

	if (strcmp(programType, RASTERIZATION) == 0) {

		std::vector<ref<HLSLShader>> shaders;

		if (properties.contains("vsMain")) {
			std::string stageError;
			std::wstring wMain = CharToString(properties["vsMain"].as_string().c_str());
			m_compileArguments[m_mainIndex] = (WCHAR*)(wMain.c_str());
			
			std::string target("vs_");
			target += properties["model"].as_string().c_str();
			std::wstring wTarget = CharToString(target.c_str());
			m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());
			m_compileArguments[11] = (WCHAR*)L"_DX12_VERTEX_STAGE";
			auto vertexShader = CompileShaderStage(&sourceBuffer, DX12::VERTEX_SHADER, stageError);
			
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
			m_compileArguments[11] = (WCHAR*)L"_DX12_GEOMETRY_STAGE";
			auto geometryShader = CompileShaderStage(&sourceBuffer, DX12::GEOMETRY_SHADER, stageError);

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
			m_compileArguments[11] = (WCHAR*)L"_DX12_PIXEL_STAGE";
			auto pixelShader = CompileShaderStage(&sourceBuffer, DX12::PIXEL_SHADER, stageError);

			if (pixelShader == nullptr) {
				error = "Error in compiling Pixel Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(pixelShader);
		}

		finalProgram = std::make_shared<Rasterization::HLSLRasterizationProgram>(shaders);
	}

	else if (strcmp(programType, RAY_TRACING) == 0) {
		m_compileArguments[m_mainIndex] = (WCHAR*)L"";
		std::string target("lib_");
		target += properties["model"].as_string().c_str();
		std::wstring wTarget = CharToString(target.c_str());
		m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());
		RayTracing::HLSLRayTracingProgramProperties rayProps;

		if (properties.contains("rayGeneration"))
			rayProps.rayGenerationFunction = std::string(properties["rayGeneration"].as_string());
		if (properties.contains("intersection"))
			rayProps.intersectionFunction = std::string(properties["intersection"].as_string());
		if (properties.contains("anyHit"))
			rayProps.anyHitFunction = std::string(properties["anyHit"].as_string());
		if (properties.contains("closestHit"))
			rayProps.closestHitFunction = std::string(properties["closestHit"].as_string());
		if (properties.contains("miss"))
			rayProps.missFunction = std::string(properties["miss"].as_string());

		ComPtr<IDxcBlob> pshaderObjectData;
		ComPtr<IUnknown> shaderRefl;

		UInt32 argCount = properties.contains("rayGeneration") ? m_minArguments : m_minArguments + 2;
		m_compileArguments[11] = (WCHAR*)L"_DX12_RAY_TRACING_LOCAL";
		if (CompileInternal(&sourceBuffer, pshaderObjectData, shaderRefl, argCount, error) == false) {
			return nullptr;
		}

		ComPtr<ID3D12LibraryReflection> pLibraryReflection;
		shaderRefl->QueryInterface(IID_PPV_ARGS(&pLibraryReflection));
		finalProgram = std::make_shared<RayTracing::HLSLRayTracingProgram>((Byte*)pshaderObjectData->GetBufferPointer(), (UInt64)pshaderObjectData->GetBufferSize(), rayProps, pLibraryReflection);
	}
	
	else if (strcmp(programType, COMPUTE) == 0) {
		std::wstring wMain = CharToString(properties["csMain"].as_string().c_str());
		m_compileArguments[m_mainIndex] = (WCHAR*)(wMain.c_str());

		std::string target("cs_");
		target += properties["model"].as_string().c_str();
		std::wstring wTarget = CharToString(target.c_str());
		m_compileArguments[m_targetIndex] = (WCHAR*)(wTarget.c_str());

		ComPtr<IDxcBlob> pshaderObjectData;
		ComPtr<IUnknown> shaderRefl;
		
		if (CompileInternal(&sourceBuffer, pshaderObjectData, shaderRefl, m_minArguments, error) == false) {
			return nullptr;
		}

		ComPtr<ID3D12ShaderReflection> pShaderReflection;
		shaderRefl->QueryInterface(IID_PPV_ARGS(&pShaderReflection));
		finalProgram = std::make_shared<Compute::HLSLComputeProgram>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), pShaderReflection);
	}
	
	else {
		error = "Unknown Shader Type";
		return nullptr;
	}

	std::string rootSignatureError;

	if (finalProgram->InitializeRootSignature(m_device, rootSignatureError) == false) {
		error = "Error in Creating Root signature: " + rootSignatureError;
		return nullptr;
	}

	auto uidStr = metaData["uuid"].as_string().c_str();
	boost::uuids::string_generator gen;
	m_shaders.emplace(gen(uidStr), finalProgram);

	return finalProgram;
}

ref<QuantumEngine::Rendering::DX12::HLSLShader> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::CompileShaderStage(const DxcBuffer* sourceBuffer, DX12_Shader_Type shaderType, std::string& error)
{
	ComPtr<IDxcBlob> pshaderObjectData;
	ComPtr<IUnknown> shaderRefl;

	if (CompileInternal(sourceBuffer, pshaderObjectData, shaderRefl, m_minArguments + 2, error) == false) {
		return nullptr;
	}

	ComPtr<ID3D12ShaderReflection> pShaderReflection;
	shaderRefl->QueryInterface(IID_PPV_ARGS(&pShaderReflection));
	return std::make_shared<HLSLShader>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), shaderType, pShaderReflection);
}

bool QuantumEngine::Rendering::DX12::DX12ShaderRegistery::CompileInternal(const DxcBuffer* sourceBuffer, ComPtr<IDxcBlob>& pshaderData, ComPtr<IUnknown>& reflection, UInt32 argumentCount, std::string& error)
{
	ComPtr<IDxcResult> compileResult;
	HRESULT result;
	result = m_dxcCompiler->Compile(sourceBuffer, (LPCWSTR*)m_compileArguments.data(), argumentCount, m_includeHandler, IID_PPV_ARGS(&compileResult));

	if (FAILED(result)) {
		error = "Unknown Error when Beginning to compile";
		return false;
	}

	ComPtr<IDxcBlobUtf8> pErrors;

	result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

	if (FAILED(result)) {
		error = "Unknown Error when Beginning to compile";
		return false;
	}

	if (pErrors && pErrors->GetStringLength() > 0)
	{
		error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
		return false;
	}

	result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pshaderData), nullptr);

	if (FAILED(result)) {
		error = "Unknown Error when Obtaining Shader Bytecode";
		return false;
	}

	// Get Reflection
	ComPtr<IDxcBlob> pReflectionData;
	result = compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(pReflectionData.GetAddressOf()), nullptr);

	if (FAILED(result)) {
		error = "Unknown Error when Obtaining Reflection Bytecode";
		return false;
	}

	DxcBuffer reflectionBuffer{
		.Ptr = pReflectionData->GetBufferPointer(),
		.Size = pReflectionData->GetBufferSize(),
		.Encoding = 0,
	};

	result = m_utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));

	if (FAILED(result)) {
		error = "Unknown Error when Creating Reflection";
		return false;
	}

	return true;
}
