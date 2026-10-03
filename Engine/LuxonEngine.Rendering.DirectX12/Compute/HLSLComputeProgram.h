#pragma once
#include "pch.h"
#include "HLSLShaderProgram.h"

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12::Compute {
	class HLSLComputeProgram : public HLSLShaderProgram
	{
	public:
		HLSLComputeProgram(ComPtr<IDxcBlob>& computeShader, ComPtr<ID3D12ShaderReflection>& shaderReflection);
		
		virtual ShaderProgramType GetType() override { return ShaderProgramType::Compute; }

		/// <summary>
		/// Creates the root signature for this shader program
		/// </summary>
		/// <param name="device"></param>
		/// <returns></returns>
		virtual bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) override;

		/// <summary>
		/// Gets length of byte code
		/// </summary>
		/// <returns></returns>
		inline IDxcBlob* GetShader() const { return m_computeShader.Get(); }

	private:
		ComPtr<IDxcBlob> m_computeShader;
	};
}