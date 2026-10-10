#include "pch.h"
#include "HLSLRayTracingProgram.h"
#include "StringUtilities.h"
#include "../Core/HLSLVariableReflection.h"
#include "DXILRuntimeData.h"
#include "Core/Logger.h"
#include <algorithm>

UInt32 LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::m_programCounter = 0;

LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::HLSLRayTracingProgram(const ComPtr<IDxcBlob>& byteCode, const std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& stages, ID3D12LibraryReflection* libraryReflection)
	:m_shaderCode(byteCode)
{
	m_programCounter++;
	m_hitDesc.AnyHitShaderImport = nullptr;
	m_hitDesc.ClosestHitShaderImport = nullptr;
	m_hitDesc.IntersectionShaderImport = nullptr;

    D3D12_LIBRARY_DESC libDesc;
    libraryReflection->GetDesc(&libDesc);

	for (auto& [stage, entry] : stages) {
		if(stage == D3D12_SHVER_RAY_GENERATION_SHADER) {
			m_rayGenOriginalName = entry;
			m_rayGenExportName = L"shader_stage_" + m_rayGenOriginalName + L"_" + std::to_wstring(m_programCounter);

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_rayGenExportName.c_str(),
				.ExportToRename = m_rayGenOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});

			continue;
		}
		if(stage == D3D12_SHVER_INTERSECTION_SHADER) {
			m_intersectionOriginalName = entry;
			m_intersectionExportName = L"shader_stage_" + m_intersectionOriginalName + L"_" + std::to_wstring(m_programCounter);
			m_hitDesc.IntersectionShaderImport = m_intersectionExportName.c_str();

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_intersectionExportName.c_str(),
				.ExportToRename = m_intersectionOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
			continue;
		}
		if(stage == D3D12_SHVER_ANY_HIT_SHADER) {
			m_anyHitOriginalName = entry;
			m_anyHitExportName = L"shader_stage_" + m_anyHitOriginalName + L"_" + std::to_wstring(m_programCounter);
			m_hitDesc.AnyHitShaderImport = m_anyHitExportName.c_str();

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_anyHitExportName.c_str(),
				.ExportToRename = m_anyHitOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
			continue;
		}
		if(stage == D3D12_SHVER_CLOSEST_HIT_SHADER) {
			m_closestHitOriginalName = entry;
			m_closestHitExportName = L"shader_stage_" + m_closestHitOriginalName + L"_" + std::to_wstring(m_programCounter);
			m_hitDesc.ClosestHitShaderImport = m_closestHitExportName.c_str();

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_closestHitExportName.c_str(),
				.ExportToRename = m_closestHitOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
			continue;
		}
		if(stage == D3D12_SHVER_MISS_SHADER) {
			m_missOriginalName = entry;
			m_missExportName = L"shader_stage_" + m_missOriginalName + L"_" + std::to_wstring(m_programCounter);
			
			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_missExportName.c_str(),
				.ExportToRename = m_missOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
		}
	}

    for (int i = 0; i < libDesc.FunctionCount; i++) {
        ID3D12FunctionReflection* funcReflection = libraryReflection->GetFunctionByIndex(i);
		AddShaderReflection(m_variableReflection, funcReflection);
    }

	m_hitGroupExportName = L"hit_group_" + std::to_wstring(m_programCounter);
	m_hitDesc.HitGroupExport = m_hitGroupExportName.c_str();
	m_hitDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

	ReadShaderSizes(byteCode.Get());

	m_dxilData.DXILLibrary = D3D12_SHADER_BYTECODE{
		.pShaderBytecode = m_shaderCode->GetBufferPointer(),
		.BytecodeLength = m_shaderCode->GetBufferSize(),
	};
	m_dxilData.NumExports = m_exportDescs.size();
	m_dxilData.pExports = m_exportDescs.data();
}

void LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::ReadShaderSizes(IDxcBlob* library)
{
	std::vector<DXILFunctionData> functions;

	if (ReadDXILFunctions(library, functions) == false) {
		Logger::LogWarning("The runtime data of a ray tracing library cannot be read, the default payload and attribute sizes are used");
		return;
	}

	struct Stage {
		const std::wstring& entry;
		DXILShaderKind kind;
	};

	const Stage stages[] = {
		{ m_rayGenOriginalName, DXILShaderKind::RayGeneration },
		{ m_intersectionOriginalName, DXILShaderKind::Intersection },
		{ m_anyHitOriginalName, DXILShaderKind::AnyHit },
		{ m_closestHitOriginalName, DXILShaderKind::ClosestHit },
		{ m_missOriginalName, DXILShaderKind::Miss },
	};

	UInt32 payloadSize = 0;
	UInt32 attributeSize = 0;

	for (auto& stage : stages) {
		if (stage.entry.empty())
			continue;

		std::string entry(stage.entry.begin(), stage.entry.end());

		auto functionIt = std::find_if(functions.begin(), functions.end(), [&entry](const DXILFunctionData& function) {
			return function.name == entry || function.mangledName.find("?" + entry + "@@") != std::string::npos;
			});

		// the shader kind confirms that the fields of the runtime data are read from the right place
		if (functionIt == functions.end() || functionIt->shaderKind != (UInt32)stage.kind) {
			Logger::LogWarning("The runtime data of a ray tracing library does not match its shaders, the default payload and attribute sizes are used");
			return;
		}

		payloadSize = std::max(payloadSize, functionIt->payloadSize);
		attributeSize = std::max(attributeSize, functionIt->attributeSize);
	}

	m_payloadSize = payloadSize;
	m_attributeSize = attributeSize;
	m_hasShaderSizes = true;
}

bool LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	std::string errorMessage;
	D3D12_ROOT_SIGNATURE_FLAGS flag = m_rayGenOriginalName.empty() ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
	m_rootSignature = CreateRootSignature(device, m_variableReflection, flag, m_rootParameterLayout, errorMessage);

	if (m_rootSignature == nullptr) {
		error = errorMessage;
		return false;
	}

	m_recordOffsets.assign(m_rootParameterLayout.GetRootParameterCount(), 0);
	m_recordArgumentSize = 0;

	for (UInt32 rootIndex = 0; rootIndex < m_recordOffsets.size(); rootIndex++) {
		bool isConstants = rootIndex == m_rootParameterLayout.GetConstantDataRootIndex();
		UInt32 alignment = isConstants ? sizeof(UInt32) : sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		UInt32 size = isConstants ? m_variableReflection.constants.size : sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);

		m_recordOffsets[rootIndex] = (m_recordArgumentSize + alignment - 1) / alignment * alignment;
		m_recordArgumentSize = m_recordOffsets[rootIndex] + size;
	}
	return true;
}

std::wstring& LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::GetRayGenExportName()
{
	static std::wstring empty;
	if (m_rayGenOriginalName.empty())
		return empty;
	return m_rayGenExportName;
}

std::wstring& LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::GetMissExportName()
{
	static std::wstring empty;
	if (m_missOriginalName.empty())
		return empty;
	return m_missExportName;
}

std::vector<LPCWSTR> LuxonEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::GetExportNames() const
{
	std::vector<LPCWSTR> names;

	if (m_rayGenOriginalName.empty() == false)
		names.push_back(m_rayGenExportName.c_str());

	if (m_missOriginalName.empty() == false)
		names.push_back(m_missExportName.c_str());

	if (HasHitGroup())
		names.push_back(m_hitGroupExportName.c_str());

	return names;
}
