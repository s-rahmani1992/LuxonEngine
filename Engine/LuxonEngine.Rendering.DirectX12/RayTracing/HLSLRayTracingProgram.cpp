#include "pch.h"
#include "HLSLRayTracingProgram.h"
#include "StringUtilities.h"

UInt32 QuantumEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::m_programCounter = 0;

QuantumEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::HLSLRayTracingProgram(Byte* byteCode, UInt64 codeLength, const HLSLRayTracingProgramProperties& properties, ComPtr<ID3D12LibraryReflection>& libraryReflection)
	:m_codeLength(codeLength)
{
	m_programCounter++;
	m_hitDesc.AnyHitShaderImport = nullptr;
	m_hitDesc.ClosestHitShaderImport = nullptr;
	m_hitDesc.IntersectionShaderImport = nullptr;

	m_byteCode = new Byte[codeLength];
	std::memcpy(m_byteCode, byteCode, codeLength);

    D3D12_LIBRARY_DESC libDesc;
    libraryReflection->GetDesc(&libDesc);

    for (int i = 0; i < libDesc.FunctionCount; i++) {
        ID3D12FunctionReflection* funcReflection = libraryReflection->GetFunctionByIndex(i);
        D3D12_FUNCTION_DESC funcDesc;
        funcReflection->GetDesc(&funcDesc);
        std::string name(funcDesc.Name);

        if (properties.rayGenerationFunction.empty() == false && name.find(properties.rayGenerationFunction) != std::string::npos) { //check if ray generation function exists
			m_rayGenOriginalName = CharToString(properties.rayGenerationFunction.c_str());
			m_rayGenExportName = L"shader_stage_" + m_rayGenOriginalName + L"_" + std::to_wstring(m_programCounter);
        
			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_rayGenExportName.c_str(),
				.ExportToRename = m_rayGenOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
		}

        if (properties.intersectionFunction.empty() == false && name.find(properties.intersectionFunction) != std::string::npos) { //check if intersection function exists
			m_intersectionOriginalName = CharToString(properties.intersectionFunction.c_str());
			m_intersectionExportName = L"shader_stage_" + m_intersectionOriginalName + L"_" + std::to_wstring(m_programCounter);
			m_hitDesc.IntersectionShaderImport = m_intersectionExportName.c_str();

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_intersectionExportName.c_str(),
				.ExportToRename = m_intersectionOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
		}

		if (properties.anyHitFunction.empty() == false && name.find(properties.anyHitFunction) != std::string::npos) { //check if any-hit function exists
			m_anyHitOriginalName = CharToString(properties.anyHitFunction.c_str());
            m_anyHitExportName = L"shader_stage_" + m_anyHitOriginalName + L"_" + std::to_wstring(m_programCounter);
			m_hitDesc.AnyHitShaderImport = m_anyHitExportName.c_str();

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_anyHitExportName.c_str(),
				.ExportToRename = m_anyHitOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
		}

		if (properties.closestHitFunction.empty() == false && name.find(properties.closestHitFunction) != std::string::npos) { //check if closest-hit function exists
            m_closestHitOriginalName = CharToString(properties.closestHitFunction.c_str());
			m_closestHitExportName = L"shader_stage_" + m_closestHitOriginalName + L"_" + std::to_wstring(m_programCounter);
			m_hitDesc.ClosestHitShaderImport = m_closestHitExportName.c_str();

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_closestHitExportName.c_str(),
				.ExportToRename = m_closestHitOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
		}

		if (properties.missFunction.empty() == false && name.find(properties.missFunction) != std::string::npos) { //check if miss function exists
			m_missOriginalName = CharToString(properties.missFunction.c_str());
			m_missExportName = L"shader_stage_" + m_missOriginalName + L"_" + std::to_wstring(m_programCounter);

			m_exportDescs.push_back(D3D12_EXPORT_DESC{
				.Name = m_missExportName.c_str(),
				.ExportToRename = m_missOriginalName.c_str(),
				.Flags = D3D12_EXPORT_FLAG_NONE,
				});
		}

		m_reflection.AddShaderReflection(funcReflection);
    }

	m_hitGroupExportName = L"hit_group_" + std::to_wstring(m_programCounter);
	m_hitDesc.HitGroupExport = m_hitGroupExportName.c_str();
	m_hitDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

	m_dxilData.DXILLibrary = D3D12_SHADER_BYTECODE{
		.pShaderBytecode = m_byteCode,
		.BytecodeLength = m_codeLength,
	};
	m_dxilData.NumExports = m_exportDescs.size();
	m_dxilData.pExports = m_exportDescs.data();
}

bool QuantumEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	std::string errorMessage;
	D3D12_ROOT_SIGNATURE_FLAGS flag = m_rayGenOriginalName.empty() ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
	m_rootSignature = m_reflection.CreateRootSignature(device, flag, errorMessage);

	if (m_rootSignature == nullptr) {
		error = errorMessage;
		return false;
	}
	return true;
}

std::wstring& QuantumEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::GetRayGenExportName()
{
	static std::wstring empty;
	if (m_rayGenOriginalName.empty())
		return empty;
	return m_rayGenExportName;
}

std::wstring& QuantumEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::GetMissExportName()
{
	static std::wstring empty;
	if (m_missOriginalName.empty())
		return empty;
	return m_missExportName;
}

std::vector<LPCWSTR> QuantumEngine::Rendering::DX12::RayTracing::HLSLRayTracingProgram::GetExportNames() const
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
