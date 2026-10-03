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

namespace LuxonEngine::Rendering::DX12
{
	const std::map<ShaderProgramType, std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>> m_validStages = {
			{ ShaderProgramType::Rasterization, {
				{ D3D12_SHVER_VERTEX_SHADER, L"vs_6_6" },
				{ D3D12_SHVER_GEOMETRY_SHADER, L"gs_6_6" },
				{ D3D12_SHVER_PIXEL_SHADER, L"ps_6_6" },
			}},
			{ ShaderProgramType::Compute, {
				{ D3D12_SHVER_COMPUTE_SHADER, L"cs_6_6" },
			}},
			{ ShaderProgramType::RayTracing, {
				{ D3D12_SHVER_RAY_GENERATION_SHADER, L"rgs_6_6" },
				{ D3D12_SHVER_MISS_SHADER, L"ms_6_6" },
				{ D3D12_SHVER_CLOSEST_HIT_SHADER, L"chs_6_6" },
				{ D3D12_SHVER_ANY_HIT_SHADER, L"ahs_6_6" },
				{ D3D12_SHVER_INTERSECTION_SHADER, L"is_6_6" }
			}},
			{ ShaderProgramType::Mesh, {
				{ D3D12_SHVER_PIXEL_SHADER, L"ps_6_6" },
				{ D3D12_SHVER_AMPLIFICATION_SHADER, L"as_6_6" },
				{ D3D12_SHVER_MESH_SHADER, L"ms_6_6" }
			}} 
	};

	static std::string DemangleFunctionName(const char* name)
	{
		std::string mangled(name);

		if (mangled.size() < 2 || mangled[0] != '\x01' || mangled[1] != '?')
			return mangled;

		const size_t end = mangled.find('@', 2);
		return mangled.substr(2, end == std::string::npos ? std::string::npos : end - 2);
	}

	DX12ShaderCompiler::DX12ShaderCompiler()
	{
		m_compiler = std::make_unique<DXC::DXCCompiler>();
	}

	DX12ShaderCompiler::~DX12ShaderCompiler() = default;

	bool DX12ShaderCompiler::Initialize(const ComPtr<ID3D12Device10>& device)
	{
		std::string error;
		if (m_compiler->Initialize(error) == false) {
			return false;
		}

		m_device = device;

		return true;
	}

	ShaderProgram* DX12ShaderCompiler::CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& compileProperties, std::string& error)
	{
		const auto baseOptions = CreateCompileOptions(compileProperties.folderPath);

		std::map<D3D12_SHADER_VERSION_TYPE, std::wstring> foundStages;
		if (DiscoverStages(shaderCode, codeLength, baseOptions, compileProperties.type, foundStages, error) == false) {
			return nullptr;
		}

		HLSLShaderProgram* finalProgram;

		if (compileProperties.type == ShaderProgramType::Rasterization) {
			std::vector<HLSLShaderData> shaders;

			for (auto& [stage, entryPoint] : foundStages) {
				const std::string stageName = ShaderStageToString(stage);

				std::string stageError;
				auto options = baseOptions;
				options.entryPoint = entryPoint.c_str();
				options.targetProfile = m_validStages.at(ShaderProgramType::Rasterization).at(stage);
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
			RayTracing::HLSLRayTracingProgramProperties rayProps;

			for (auto& [stage, entryPoint] : foundStages) {
				const std::string functionName = WCharToString(entryPoint.c_str());

				switch (stage) {
					case D3D12_SHVER_RAY_GENERATION_SHADER:
						rayProps.rayGenerationFunction = functionName;
						break;
					case D3D12_SHVER_INTERSECTION_SHADER:
						rayProps.intersectionFunction = functionName;
						break;
					case D3D12_SHVER_ANY_HIT_SHADER:
						rayProps.anyHitFunction = functionName;
						break;
					case D3D12_SHVER_CLOSEST_HIT_SHADER:
						rayProps.closestHitFunction = functionName;
						break;
					case D3D12_SHVER_MISS_SHADER:
						rayProps.missFunction = functionName;
						break;
				}
			}

			// The whole file is compiled as a single library. Programs without a ray generation stage use local root signatures,
			// which changes the register space, so the library is compiled again with the local define once the stages are known
			auto options = baseOptions;
			options.targetProfile = CharToString("lib_6_6");

			if (rayProps.rayGenerationFunction.empty())
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

			finalProgram = new RayTracing::HLSLRayTracingProgram(pshaderObjectData, foundStages, pLibraryReflection.Get());
		}

		else if (compileProperties.type == ShaderProgramType::Compute) {
			const auto& [stage, entryPoint] = *foundStages.begin();

			std::string stageError;
			auto options = baseOptions;
			options.entryPoint = entryPoint;
			options.targetProfile = m_validStages.at(ShaderProgramType::Compute).at(stage);
			auto shader = CompileShaderStageWithReflection(shaderCode, codeLength, options, stageError);

			if (shader.byteCode == nullptr) {
				error = std::string("Error in compiling ") + ShaderStageToString(stage) + " Stage: " + stageError;
				return nullptr;
			}

			finalProgram = new Compute::HLSLComputeProgram(shader.byteCode, shader.reflection);
		}

		else if (compileProperties.type == ShaderProgramType::Mesh) {
			std::vector<HLSLShaderData> shaders;

			for (auto& [stage, entryPoint] : foundStages) {
				if (entryPoint.empty())
					continue;

				std::string stageError;
				auto options = baseOptions;
				options.entryPoint = entryPoint;
				options.targetProfile = m_validStages.at(ShaderProgramType::Mesh).at(stage);
				auto hlslShader = CompileShaderStageWithReflection(shaderCode, codeLength, options, stageError);

				if (hlslShader.byteCode == nullptr) {
					error = std::string("Error in compiling ") + ShaderStageToString(stage) + " Stage: " + stageError;
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

	DXC::DXCCompileOptions DX12ShaderCompiler::CreateCompileOptions(const std::wstring& includeDir) const
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

	bool DX12ShaderCompiler::DiscoverStages(const void* source, size_t size, const DXC::DXCCompileOptions& baseOptions, const ShaderProgramType shaderType, std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& outStages, std::string& error)
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

		auto& validStages = m_validStages.at(shaderType);

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

			outStages[stage] = CharToString(DemangleFunctionName(functionDesc.Name).c_str());
		}

		if (outStages.empty()) {
			error = "No shader stage found. Entry points must be marked with the [shader(\"...\")] attribute";
			return false;
		}

		return true;
	}

	HLSLShaderData DX12ShaderCompiler::CompileShaderStageWithReflection(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error)
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
}
