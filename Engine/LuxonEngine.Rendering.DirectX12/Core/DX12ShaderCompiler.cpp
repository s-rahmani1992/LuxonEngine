#include "pch.h"
#include "DX12ShaderCompiler.h"
#include "DXCCompiler.h"
#include <fstream>
#include <filesystem>

#include "Rasterization/HLSLRasterizationProgram.h"
#include "RayTracing/HLSLRayTracingProgram.h"
#include "Compute/HLSLComputeProgram.h"
#include "Mesh/HLSLMeshProgram.h"
#include "HLSLVariableReflection.h"

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

	return true;
}

LuxonEngine::Rendering::ShaderProgram* LuxonEngine::Rendering::DX12::DX12ShaderCompiler::CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& compileProperties, std::string& error)
{
	const auto baseOptions = CreateCompileOptions(compileProperties.folderPath);

	HLSLShaderProgram* finalProgram;

	if (compileProperties.type == ShaderProgramType::Rasterization) {
		std::map<D3D12_SHADER_VERSION_TYPE, std::wstring> foundStages;

		if (DiscoverStages(shaderCode, codeLength, baseOptions, m_validRasterStages, foundStages, error) == false) {
			return nullptr;
		}

		std::vector<HLSLShaderData> shaders;

		for (auto& [stage, entryPoint] : foundStages) {
			const std::string stageName = ShaderStageToString(stage);

			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = entryPoint.c_str();
			options.targetProfile = m_validRasterStages.at(stage);
			auto shader = CompileShaderStageWithReflection(shaderCode, codeLength, options, stageError);

			if (shader.byteCode == nullptr) {
				error = std::string("Error in compiling ") + stageName + " Stage: " + stageError;
				return nullptr;
			}
			shader.shaderType = stage;
			shader.entryPoint = WCharToString(entryPoint.c_str());
			shaders.push_back(shader);
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
		std::map<D3D12_SHADER_VERSION_TYPE, std::wstring> foundStages;

		if (DiscoverStages(shaderCode, codeLength, baseOptions, m_validComputeStages, foundStages, error) == false) {
			return nullptr;
		}

		const auto& [stage, entryPoint] = *foundStages.begin();

		std::string stageError;
		auto options = baseOptions;
		options.entryPoint = entryPoint;
		options.targetProfile = m_validComputeStages.at(stage);
		auto shader = CompileShaderStageWithReflection(shaderCode, codeLength, options, stageError);

		if (shader.byteCode == nullptr) {
			error = std::string("Error in compiling ") + ShaderStageToString(stage) + " Stage: " + stageError;
			return nullptr;
		}

		finalProgram = new Compute::HLSLComputeProgram(shader.byteCode, shader.reflection);
	}

	else if (compileProperties.type == ShaderProgramType::Mesh) {
		std::map<D3D12_SHADER_VERSION_TYPE, std::wstring> foundStages;

		if (DiscoverStages(shaderCode, codeLength, baseOptions, m_validMeshStages, foundStages, error) == false) {
			return nullptr;
		}

		std::vector<HLSLShaderData> shaders;

		for (auto& [stage, entryPoint] : foundStages) {
			if (entryPoint.empty())
				continue;

			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = entryPoint;
			options.targetProfile = m_validMeshStages[stage];
			auto hlslShader = CompileShaderStageWithReflection(shaderCode, codeLength, options, stageError);

			if (hlslShader.byteCode == nullptr) {
				error = std::string("Error in compiling ") +  ShaderStageToString(stage) + " Stage: " + stageError;
				return nullptr;
			}
			hlslShader.shaderType = stage;
			hlslShader.entryPoint = WCharToString(entryPoint.c_str());
			shaders.push_back(hlslShader);
		}

		finalProgram = new MeshShading::HLSLMeshProgram(shaders);
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

bool LuxonEngine::Rendering::DX12::DX12ShaderCompiler::DiscoverStages(const void* source, size_t size, const DXC::DXCCompileOptions& baseOptions, const std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& validStages, std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& outStages, std::string& error)
{
	auto libraryOptions = baseOptions;
	libraryOptions.targetProfile = CharToString("lib_6_6");

	ComPtr<IDxcBlob> pLibraryObjectData;
	ComPtr<IDxcBlob> pLibraryReflectionData;

	if (m_compiler->Compile(source, size, libraryOptions, pLibraryObjectData, error, &pLibraryReflectionData) == false) {
		return false;
	}

	ComPtr<ID3D12LibraryReflection> pLibraryReflection;
	if (FAILED(m_compiler->CreateReflection(pLibraryReflectionData.Get(), IID_PPV_ARGS(&pLibraryReflection)))) {
		error = "Unknown Error when Creating Reflection";
		return false;
	}

	D3D12_LIBRARY_DESC libraryDesc;
	pLibraryReflection->GetDesc(&libraryDesc);

	for (UINT i = 0; i < libraryDesc.FunctionCount; i++) {
		D3D12_FUNCTION_DESC functionDesc;
		pLibraryReflection->GetFunctionByIndex(i)->GetDesc(&functionDesc);

		D3D12_SHADER_VERSION_TYPE stage = (D3D12_SHADER_VERSION_TYPE)D3D12_SHVER_GET_TYPE(functionDesc.Version);

		// Functions that are not entry points of any stage
		if (stage == D3D12_SHVER_RESERVED0)
			continue;

		if (validStages.find(stage) == validStages.end()) {
			error = std::string("Invalid Stage Found: ") + ShaderStageToString(stage) + " stage is not supported in this shader type";
			return false;
		}

		if (outStages.find(stage) != outStages.end()) {
			error = std::string("Multiple ") + ShaderStageToString(stage) + " Stage Found";
			return false;
		}

		outStages[stage] = CharToString(functionDesc.Name);
	}

	if (outStages.empty()) {
		error = "No shader stage found. Entry points must be marked with the [shader(\"...\")] attribute";
		return false;
	}

	return true;
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

LuxonEngine::Rendering::DX12::HLSLShaderData LuxonEngine::Rendering::DX12::DX12ShaderCompiler::CompileShaderStageWithReflection(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error)
{
	ComPtr<IDxcBlob> pshaderObjectData;
	ComPtr<IDxcBlob> pReflectionData;

	HLSLShaderData shaderData;

	if (m_compiler->Compile(source, size, options, pshaderObjectData, error, &pReflectionData) == false) {
		return shaderData;
	}

	ComPtr<ID3D12ShaderReflection> pShaderReflection;
	if (FAILED(m_compiler->CreateReflection(pReflectionData.Get(), IID_PPV_ARGS(&pShaderReflection)))) {
		error = "Unknown Error when Creating Reflection";
		return shaderData;
	}

	return HLSLShaderData{
		.byteCode = pshaderObjectData,
		.reflection = pShaderReflection
	};
}
