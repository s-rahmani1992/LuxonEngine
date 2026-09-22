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

	return true;
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
