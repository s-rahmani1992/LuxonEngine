#pragma once
#include "pch.h"
#include <string>
#include "HLSLShaderProgram.h"

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12::MeshShading {
	class HLSLMeshProgram : public HLSLShaderProgram {
	public:
		HLSLMeshProgram(const std::vector<HLSLShaderData>& shaders);
		virtual ~HLSLMeshProgram() = default;

		virtual ShaderProgramType GetType() override { return ShaderProgramType::Mesh; }
		bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) override;

		inline IDxcBlob* GetAmplificationShader() const { return m_amplificationShader.Get(); }
		inline IDxcBlob* GetMeshShader() const { return m_meshShader.Get(); }
		inline IDxcBlob* GetPixelShader() const { return m_pixelShader.Get(); }

		D3D12_SHADER_BYTECODE GetAmplificationShaderBytecode() const;
		D3D12_SHADER_BYTECODE GetMeshShaderBytecode() const;
		D3D12_SHADER_BYTECODE GetPixelShaderBytecode() const;

		D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

	private:
		ComPtr<IDxcBlob> m_amplificationShader = nullptr;
		ComPtr<IDxcBlob> m_meshShader = nullptr;
		ComPtr<IDxcBlob> m_pixelShader = nullptr;

		RenderTargetReflection m_renderTargetReflection;
		ShaderThreadGroupReflection m_threadGroupReflection;
	};
}
