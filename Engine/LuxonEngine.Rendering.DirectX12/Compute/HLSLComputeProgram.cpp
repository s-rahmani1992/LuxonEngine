#include "pch.h"
#include "HLSLComputeProgram.h"

LuxonEngine::Rendering::DX12::Compute::HLSLComputeProgram::HLSLComputeProgram(ComPtr<IDxcBlob>& computeShader, ComPtr<ID3D12ShaderReflection>& shaderReflection)
    :m_computeShader(computeShader)
{
	m_reflection.AddShaderReflection(shaderReflection.Get());
}

bool LuxonEngine::Rendering::DX12::Compute::HLSLComputeProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	std::string errorMessage;
	m_rootSignature = m_reflection.CreateRootSignature(device, D3D12_ROOT_SIGNATURE_FLAG_NONE, errorMessage);

	return m_rootSignature != nullptr;
}
