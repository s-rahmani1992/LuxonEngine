#pragma once
#include "pch.h"
#include "HLSLShaderProgram.h"

namespace LuxonEngine::Rendering::DX12::RayTracing {
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
		HLSLRayTracingProgram(const ComPtr<IDxcBlob>& byteCode, const std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& stages, ID3D12LibraryReflection* shaderReflection);
		
		virtual ShaderProgramType GetType() override { return ShaderProgramType::RayTracing; }

		/// <summary>
		/// Creates Root signature from this Ray tracing program
		/// </summary>
		/// <param name="device"></param>
		/// <returns></returns>
		bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) override;

		/// <summary>
		/// Gets the offset of the argument of a root parameter from the start of the arguments of a shader record. it is created along with the root signature
		/// </summary>
		/// <param name="rootParameterIndex"></param>
		/// <returns></returns>
		UInt32 GetRecordOffset(UInt32 rootParameterIndex) const { return m_recordOffsets[rootParameterIndex]; }

		/// <summary>
		/// Gets the size in bytes of all the arguments of a shader record, without the shader identifier
		/// </summary>
		/// <returns></returns>
		UInt32 GetRecordArgumentSize() const { return m_recordArgumentSize; }

		/// <summary>
		/// Determines if the payload and attribute sizes of this program are known. They are read from the runtime data of the library
		/// </summary>
		bool HasShaderSizes() const { return m_hasShaderSizes; }

		/// <summary>
		/// Gets the largest payload size in bytes of the shaders of this program
		/// </summary>
		UInt32 GetPayloadSize() const { return m_payloadSize; }

		/// <summary>
		/// Gets the largest hit attribute size in bytes of the shaders of this program
		/// </summary>
		UInt32 GetAttributeSize() const { return m_attributeSize; }

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
		void ReadShaderSizes(IDxcBlob* library);

		static UInt32 m_programCounter;
		ComPtr<IDxcBlob> m_shaderCode;

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

		std::vector<UInt32> m_recordOffsets; // root parameter index -> offset of its argument in a shader record
		UInt32 m_recordArgumentSize = 0;

		bool m_hasShaderSizes = false;
		UInt32 m_payloadSize = 0;
		UInt32 m_attributeSize = 0;

		D3D12_DXIL_LIBRARY_DESC m_dxilData;
		std::vector<D3D12_EXPORT_DESC> m_exportDescs;
	};
}