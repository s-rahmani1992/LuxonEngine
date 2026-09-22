#include "pch.h"
#include "DX12ShaderCompiler.h"
#include "DXCCompiler.h"
#include <fstream>
#include <filesystem>

#include "Rasterization/HLSLRasterizationProgram.h"
#include "RayTracing/HLSLRayTracingProgram.h"
#include "Compute/HLSLComputeProgram.h"

#include "StringUtilities.h"
#include <vector>
#include <memory>
#include <Platform/Application.h>

#include <boost/uuid/string_generator.hpp>
#include <boost/json.hpp>

#define RASTERIZATION "Rasterization"
#define RAY_TRACING "RayTracing"
#define COMPUTE "Compute"

namespace Render = LuxonEngine::Rendering;
namespace HLSL = LuxonEngine::Rendering::DX12::Rasterization;
namespace Compute = LuxonEngine::Rendering::DX12::Compute;

LuxonEngine::Rendering::DX12::DX12ShaderCompiler::DX12ShaderCompiler()
{
	m_compiler = std::make_unique<DXC::DXCCompiler>();
}

LuxonEngine::Rendering::DX12::DX12ShaderCompiler::~DX12ShaderCompiler() = default;

bool LuxonEngine::Rendering::DX12::DX12ShaderCompiler::Initialize(const ComPtr<ID3D12Device10>& device)
{
	std::string error;
	if (m_compiler->Initialize(error) == false) {
		return false;
	}

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

	return true;
}

ref<LuxonEngine::Rendering::DX12::HLSLShaderProgram> LuxonEngine::Rendering::DX12::DX12ShaderCompiler::GetShaderProgram(const std::string& name)
{
	auto it = m_specialShaders.find(name);
	if (it != m_specialShaders.end())
		return (*it).second;
	return nullptr;
}

void LuxonEngine::Rendering::DX12::DX12ShaderCompiler::RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT)
{
	ref<HLSLShaderProgram> hlslProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(program);

	if(hlslProgram != nullptr) {
		m_specialShaders.emplace(name, hlslProgram);
	}
}

ref<LuxonEngine::Rendering::ShaderProgram> LuxonEngine::Rendering::DX12::DX12ShaderCompiler::CompileProgram(const std::wstring& hlslFile, std::string& error)
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

	auto path = std::filesystem::path(hlslFile);
	std::wstring shaderDir = path.parent_path().c_str(); /* extract directory from fileName */;

	const auto baseOptions = CreateCompileOptions(shaderDir);

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
	std::string model = properties["model"].as_string().c_str();

	ref<HLSLShaderProgram> finalProgram;

	if (strcmp(programType, RASTERIZATION) == 0) {

		std::vector<ref<HLSLShader>> shaders;

		if (properties.contains("vsMain")) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(properties["vsMain"].as_string().c_str());
			options.targetProfile = CharToString(("vs_" + model).c_str());
			options.defines.push_back(L"_DX12_VERTEX_STAGE");
			auto vertexShader = CompileShaderStage(buffer.data(), buffer.size(), options, DX12::VERTEX_SHADER, stageError);

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
			options.defines.push_back(L"_DX12_GEOMETRY_STAGE");
			auto geometryShader = CompileShaderStage(buffer.data(), buffer.size(), options, DX12::GEOMETRY_SHADER, stageError);

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
			options.defines.push_back(L"_DX12_PIXEL_STAGE");
			auto pixelShader = CompileShaderStage(buffer.data(), buffer.size(), options, DX12::PIXEL_SHADER, stageError);

			if (pixelShader == nullptr) {
				error = "Error in compiling Pixel Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(pixelShader);
		}

		finalProgram = std::make_shared<Rasterization::HLSLRasterizationProgram>(shaders);
	}

	else if (strcmp(programType, RAY_TRACING) == 0) {
		auto options = baseOptions;
		options.targetProfile = CharToString(("lib_" + model).c_str());
		RayTracing::HLSLRayTracingProgramProperties rayProps;

		if (properties.contains("rayGen"))
			rayProps.rayGenerationFunction = std::string(properties["rayGen"].as_string());
		if (properties.contains("intersection"))
			rayProps.intersectionFunction = std::string(properties["intersection"].as_string());
		if (properties.contains("anyHit"))
			rayProps.anyHitFunction = std::string(properties["anyHit"].as_string());
		if (properties.contains("closestHit"))
			rayProps.closestHitFunction = std::string(properties["closestHit"].as_string());
		if (properties.contains("miss"))
			rayProps.missFunction = std::string(properties["miss"].as_string());

		if (!properties.contains("rayGen"))
			options.defines.push_back(L"_DX12_RAY_TRACING_LOCAL");

		ComPtr<IDxcBlob> pshaderObjectData;
		ComPtr<IDxcBlob> pReflectionData;

		if (m_compiler->Compile(buffer.data(), buffer.size(), options, pshaderObjectData, error, &pReflectionData) == false) {
			return nullptr;
		}

		ComPtr<ID3D12LibraryReflection> pLibraryReflection;
		if (FAILED(m_compiler->CreateReflection(pReflectionData.Get(), IID_PPV_ARGS(&pLibraryReflection)))) {
			error = "Unknown Error when Creating Reflection";
			return nullptr;
		}

		finalProgram = std::make_shared<RayTracing::HLSLRayTracingProgram>((Byte*)pshaderObjectData->GetBufferPointer(), (UInt64)pshaderObjectData->GetBufferSize(), rayProps, pLibraryReflection);
	}

	else if (strcmp(programType, COMPUTE) == 0) {
		auto options = baseOptions;
		options.entryPoint = CharToString(properties["csMain"].as_string().c_str());
		options.targetProfile = CharToString(("cs_" + model).c_str());

		ComPtr<IDxcBlob> pshaderObjectData;
		ComPtr<IDxcBlob> pReflectionData;

		if (m_compiler->Compile(buffer.data(), buffer.size(), options, pshaderObjectData, error, &pReflectionData) == false) {
			return nullptr;
		}

		ComPtr<ID3D12ShaderReflection> pShaderReflection;
		if (FAILED(m_compiler->CreateReflection(pReflectionData.Get(), IID_PPV_ARGS(&pShaderReflection)))) {
			error = "Unknown Error when Creating Reflection";
			return nullptr;
		}

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

LuxonEngine::Rendering::ShaderProgram* LuxonEngine::Rendering::DX12::DX12ShaderCompiler::CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& compileProperties, std::string& error)
{
	const auto baseOptions = CreateCompileOptions(compileProperties.folderPath);

	HLSLShaderProgram* finalProgram;

	if (compileProperties.type == ShaderProgramType::Rasterization) {

		std::vector<ref<HLSLShader>> shaders;

		if (compileProperties.rasterProperties.vertexMain != nullptr) {
			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = CharToString(compileProperties.rasterProperties.vertexMain);
			options.targetProfile = CharToString(("vs_" + compileProperties.model).c_str());
			options.defines.push_back(L"_DX12_VERTEX_STAGE");
			auto vertexShader = CompileShaderStage(shaderCode, codeLength, options, DX12::VERTEX_SHADER, stageError);

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
			options.defines.push_back(L"_DX12_GEOMETRY_STAGE");
			auto geometryShader = CompileShaderStage(shaderCode, codeLength, options, DX12::GEOMETRY_SHADER, stageError);

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
			options.defines.push_back(L"_DX12_PIXEL_STAGE");
			auto pixelShader = CompileShaderStage(shaderCode, codeLength, options, DX12::PIXEL_SHADER, stageError);

			if (pixelShader == nullptr) {
				error = "Error in compiling Pixel Stage: " + stageError;
				return nullptr;
			}

			shaders.push_back(pixelShader);
		}

		finalProgram = new Rasterization::HLSLRasterizationProgram(shaders);
	}

	else if (compileProperties.type == ShaderProgramType::RayTracing) {
		auto options = baseOptions;
		options.targetProfile = CharToString(("lib_" + compileProperties.model).c_str());
		RayTracing::HLSLRayTracingProgramProperties rayProps;

		if (compileProperties.rayTracingProperties.rayGen != nullptr)
			rayProps.rayGenerationFunction = std::string(compileProperties.rayTracingProperties.rayGen);
		if (compileProperties.rayTracingProperties.intersection)
			rayProps.intersectionFunction = std::string(compileProperties.rayTracingProperties.intersection);
		if (compileProperties.rayTracingProperties.anyHit)
			rayProps.anyHitFunction = std::string(compileProperties.rayTracingProperties.anyHit);
		if (compileProperties.rayTracingProperties.closestHit)
			rayProps.closestHitFunction = std::string(compileProperties.rayTracingProperties.closestHit);
		if (compileProperties.rayTracingProperties.miss)
			rayProps.missFunction = std::string(compileProperties.rayTracingProperties.miss);

		if (compileProperties.rayTracingProperties.rayGen == nullptr)
			options.defines.push_back(L"_DX12_RAY_TRACING_LOCAL");

		ComPtr<IDxcBlob> pshaderObjectData;
		ComPtr<IDxcBlob> pReflectionData;

		if (m_compiler->Compile(shaderCode, codeLength, options, pshaderObjectData, error, &pReflectionData) == false) {
			return nullptr;
		}

		ComPtr<ID3D12LibraryReflection> pLibraryReflection;
		if (FAILED(m_compiler->CreateReflection(pReflectionData.Get(), IID_PPV_ARGS(&pLibraryReflection)))) {
			error = "Unknown Error when Creating Reflection";
			return nullptr;
		}

		finalProgram = new RayTracing::HLSLRayTracingProgram((Byte*)pshaderObjectData->GetBufferPointer(), (UInt64)pshaderObjectData->GetBufferSize(), rayProps, pLibraryReflection);
	}

	else if (compileProperties.type == ShaderProgramType::Compute) {
		auto options = baseOptions;
		options.entryPoint = CharToString(compileProperties.computeProperties.computeMain);
		options.targetProfile = CharToString(("cs_" + compileProperties.model).c_str());

		ComPtr<IDxcBlob> pshaderObjectData;
		ComPtr<IDxcBlob> pReflectionData;

		if (m_compiler->Compile(shaderCode, codeLength, options, pshaderObjectData, error, &pReflectionData) == false) {
			return nullptr;
		}

		ComPtr<ID3D12ShaderReflection> pShaderReflection;
		if (FAILED(m_compiler->CreateReflection(pReflectionData.Get(), IID_PPV_ARGS(&pShaderReflection)))) {
			error = "Unknown Error when Creating Reflection";
			return nullptr;
		}

		finalProgram = new Compute::HLSLComputeProgram((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), pShaderReflection);
	}

	else {
		error = "Unknown Shader Type";
		return nullptr;
	}

	std::string rootSignatureError;

	if (finalProgram->InitializeRootSignature(m_device, rootSignatureError) == false) {
		error = "Error in Creating Root signature: " + rootSignatureError;
		delete finalProgram;
		return nullptr;
	}

	return finalProgram;
}

ref<LuxonEngine::Rendering::ShaderProgram> LuxonEngine::Rendering::DX12::DX12ShaderCompiler::GetProgramByGUID(boost::uuids::uuid guid)
{
	auto it = m_shaders.find(guid);

	if (it != m_shaders.end())
		return it->second;

	return nullptr;
}

LuxonEngine::Rendering::DXC::DXCCompileOptions LuxonEngine::Rendering::DX12::DX12ShaderCompiler::CreateCompileOptions(const std::wstring& includeDir) const
{
	DXC::DXCCompileOptions options;
	options.includeDirs.push_back(includeDir);

	// Strip reflection data and pdbs, reflection is obtained separately
	options.arguments = {
		L"-Qstrip_debug",
		L"-Qstrip_reflect",
		DXC_ARG_WARNINGS_ARE_ERRORS, //-WX
		DXC_ARG_DEBUG, //-Zi
	};

	options.outputReflection = true;
	return options;
}

ref<LuxonEngine::Rendering::DX12::HLSLShader> LuxonEngine::Rendering::DX12::DX12ShaderCompiler::CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, DX12_Shader_Type shaderType, std::string& error)
{
	ComPtr<IDxcBlob> pshaderObjectData;
	ComPtr<IDxcBlob> pReflectionData;

	if (m_compiler->Compile(source, size, options, pshaderObjectData, error, &pReflectionData) == false) {
		return nullptr;
	}

	ComPtr<ID3D12ShaderReflection> pShaderReflection;
	if (FAILED(m_compiler->CreateReflection(pReflectionData.Get(), IID_PPV_ARGS(&pShaderReflection)))) {
		error = "Unknown Error when Creating Reflection";
		return nullptr;
	}

	return std::make_shared<HLSLShader>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), shaderType, pShaderReflection);
}
