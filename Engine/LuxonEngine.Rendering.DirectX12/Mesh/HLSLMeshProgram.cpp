#include "pch.h"
#include "HLSLMeshProgram.h"#
#include "HLSLVariableReflection.h"

namespace LuxonEngine::Rendering::DX12::MeshShading {

	LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::HLSLMeshProgram(const std::vector<HLSLShaderData>& shaders)
	{
		for (auto& shader : shaders) {
			if (shader.shaderType == D3D12_SHVER_AMPLIFICATION_SHADER) {
				m_amplificationShader = shader.byteCode;
			}
			else if (shader.shaderType == D3D12_SHVER_MESH_SHADER) {
				m_meshShader = shader.byteCode;
			}
			else if (shader.shaderType == D3D12_SHVER_PIXEL_SHADER) {
				m_pixelShader = shader.byteCode;
				DX12::FillRenderTargetReflection(m_renderTargetReflection, shader.reflection.Get());
			}

			DX12::AddShaderReflection(m_variableReflection, shader.reflection.Get());
		}
	}

	bool LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
	{
		m_rootSignature = DX12::CreateRootSignature(device, m_variableReflection, D3D12_ROOT_SIGNATURE_FLAG_NONE, m_rootParameterLayout, error);
		return m_rootSignature != nullptr;
	}

	D3D12_SHADER_BYTECODE LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::GetAmplificationShaderBytecode() const
	{
		if (m_amplificationShader == nullptr) {
			return D3D12_SHADER_BYTECODE{};
		}
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = m_amplificationShader->GetBufferPointer(),
			.BytecodeLength = m_amplificationShader->GetBufferSize(),
		};
	}

	D3D12_SHADER_BYTECODE LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::GetMeshShaderBytecode() const
	{
		if (m_meshShader == nullptr) {
			return D3D12_SHADER_BYTECODE{};
		}
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = m_meshShader->GetBufferPointer(),
			.BytecodeLength = m_meshShader->GetBufferSize(),
		};
	}

	D3D12_SHADER_BYTECODE LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::GetPixelShaderBytecode() const
	{
		if (m_pixelShader == nullptr) {
			return D3D12_SHADER_BYTECODE{};
		}
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = m_pixelShader->GetBufferPointer(),
			.BytecodeLength = m_pixelShader->GetBufferSize(),
		};
	}

	D3D12_RT_FORMAT_ARRAY LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::GetRenderTargetFormats() const
	{
		D3D12_RT_FORMAT_ARRAY formatArray{
					.NumRenderTargets = (UINT)m_renderTargetReflection.formats.size(),
		};

		for (int i = 0; i < m_renderTargetReflection.formats.size() && i < 8; ++i) {
			formatArray.RTFormats[i] = m_renderTargetReflection.formats[i];
		}

		for (int i = m_renderTargetReflection.formats.size(); i < 8; ++i) {
			formatArray.RTFormats[i] = DXGI_FORMAT_UNKNOWN;
		}

		return formatArray;
	}
}