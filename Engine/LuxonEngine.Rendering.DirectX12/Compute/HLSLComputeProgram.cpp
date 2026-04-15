#include "pch.h"
#include "HLSLComputeProgram.h"

QuantumEngine::Rendering::DX12::Compute::HLSLComputeProgram::HLSLComputeProgram(Byte* byteCode, UInt64 codeLength, ComPtr<ID3D12ShaderReflection>& shaderReflection)
    :m_codeLength(codeLength)
{
    m_byteCode = new Byte[codeLength];
    std::memcpy(m_byteCode, byteCode, codeLength);

	m_reflection.AddShaderReflection(shaderReflection.Get());
}

bool QuantumEngine::Rendering::DX12::Compute::HLSLComputeProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	std::string errorMessage;
	m_rootSignature = m_reflection.CreateRootSignature(device, D3D12_ROOT_SIGNATURE_FLAG_NONE, errorMessage);

	return m_rootSignature != nullptr;
}
