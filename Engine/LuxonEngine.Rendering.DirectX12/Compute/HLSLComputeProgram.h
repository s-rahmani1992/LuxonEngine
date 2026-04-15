#pragma once
#include "pch.h"
#include "HLSLShaderProgram.h"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12::Compute {
	class HLSLComputeProgram : public HLSLShaderProgram
	{
	public:
		HLSLComputeProgram(Byte* byteCode, UInt64 codeLength, ComPtr<ID3D12ShaderReflection>& shaderReflection);
		
		/// <summary>
		/// Creates the root signature for this shader program
		/// </summary>
		/// <param name="device"></param>
		/// <returns></returns>
		virtual bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) override;

		/// <summary>
		/// Gets pointer to byte code
		/// </summary>
		/// <returns></returns>
		inline Byte* GetByteCode() const { return m_byteCode; }

		/// <summary>
		/// Gets length of byte code
		/// </summary>
		/// <returns></returns>
		inline UInt64 GetCodeLength() const { return m_codeLength; }

	private:
		Byte* m_byteCode;
		UInt64 m_codeLength;
	};
}