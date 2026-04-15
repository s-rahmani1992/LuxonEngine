#pragma once
#include "pch.h"
#include "HLSLShaderProgram.h"

namespace QuantumEngine::Rendering::DX12::RayTracing {
	struct HLSLRayTracingProgramProperties;

	struct HLSLRayTracingProgramProperties {
		std::string shaderModel;
		std::string rayGenerationFunction;
		std::string intersectionFunction;
		std::string anyHitFunction;
		std::string closestHitFunction;
		std::string missFunction;
	};

	class HLSLRayTracingProgram : public HLSLShaderProgram {
	public:
		HLSLRayTracingProgram(Byte* byteCode, UInt64 codeLength, const HLSLRayTracingProgramProperties& properties, ComPtr<ID3D12LibraryReflection>& shaderReflection);
		
		/// <summary>
		/// Creates Root signature from this Ray tracing program
		/// </summary>
		/// <param name="device"></param>
		/// <returns></returns>
		bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) override;

		/// <summary>
		/// Gets pointer to DXIL data of this program
		/// </summary>
		/// <returns></returns>
		D3D12_DXIL_LIBRARY_DESC* GetDXIL() { return &m_dxilData; }

		/// <summary>
		/// Gets name of the exported Ray Generation function. returns empty if it does not exist
		/// </summary>
		/// <returns></returns>
		std::wstring& GetRayGenExportName();

		/// <summary>
		/// Gets name of the exported Miss function. returns empty if it does not exist
		/// </summary>
		/// <returns></returns>
		std::wstring& GetMissExportName();

		/// <summary>
		/// Gets list of exported names of the existing ray tracing stages
		/// </summary>
		/// <returns></returns>
		std::vector<LPCWSTR> GetExportNames() const;

		/// <summary>
		/// Gets pointer to hit export description data
		/// </summary>
		/// <returns></returns>
		D3D12_HIT_GROUP_DESC* GetHitGroupDesc() { return &m_hitDesc; }

		/// <summary>
		/// determines if this program has any hit group stages
		/// </summary>
		/// <returns></returns>
		bool HasHitGroup() const {
			return m_closestHitOriginalName.empty() == false || 
				m_anyHitOriginalName.empty() == false || 
				m_intersectionOriginalName.empty() == false;
		}

		/// <summary>
		/// determines if this program has miss stage
		/// </summary>
		/// <returns></returns>
		bool HasMissStage() { return m_missOriginalName.empty() == false; }

	private:
		static UInt32 m_programCounter;
		Byte* m_byteCode;
		UInt64 m_codeLength;

		std::wstring m_rayGenOriginalName;
		std::wstring m_rayGenExportName;

		std::wstring m_anyHitOriginalName;
		std::wstring m_anyHitExportName;

		std::wstring m_intersectionOriginalName;
		std::wstring m_intersectionExportName;

		std::wstring m_closestHitOriginalName;
		std::wstring m_closestHitExportName;

		std::wstring m_missOriginalName;
		std::wstring m_missExportName;

		std::wstring m_hitGroupExportName;

		D3D12_HIT_GROUP_DESC m_hitDesc;

		D3D12_DXIL_LIBRARY_DESC m_dxilData;
		std::vector<D3D12_EXPORT_DESC> m_exportDescs;
	};
}