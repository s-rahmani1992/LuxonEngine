#include "pch.h"
#include "HLSLMeshProgram.h"#
#include "HLSLVariableReflection.h"

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
		}

		DX12::AddShaderReflection(m_variableReflection, shader.reflection.Get());
	}
}

bool LuxonEngine::Rendering::DX12::MeshShading::HLSLMeshProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	m_rootSignature = DX12::CreateRootSignature(device, m_variableReflection, D3D12_ROOT_SIGNATURE_FLAG_NONE, m_rootParameterLayout, error);
	return m_rootSignature != nullptr;
}
