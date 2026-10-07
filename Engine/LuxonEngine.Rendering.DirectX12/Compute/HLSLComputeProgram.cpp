#include "pch.h"
#include "HLSLComputeProgram.h"
#include "../Core/HLSLVariableReflection.h"

LuxonEngine::Rendering::DX12::Compute::HLSLComputeProgram::HLSLComputeProgram(ComPtr<IDxcBlob>& computeShader, ComPtr<ID3D12ShaderReflection>& shaderReflection)
    :m_computeShader(computeShader)
{
	m_reflection.AddShaderReflection(shaderReflection.Get());
	AddShaderReflection(m_variableReflection, shaderReflection.Get());
}

bool LuxonEngine::Rendering::DX12::Compute::HLSLComputeProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	m_rootSignature = CreateRootSignature(device, m_variableReflection, D3D12_ROOT_SIGNATURE_FLAG_NONE, m_rootParameterLayout, error);

	return m_rootSignature != nullptr;
}
