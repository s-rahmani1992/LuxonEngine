#include "pch.h"
#include "HLSLRasterizationProgram.h"
#include "HLSLShader.h"
#include <set>

QuantumEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram::HLSLRasterizationProgram(const std::vector<ref<HLSLShader>>& shaders)
{
    std::set<std::string> keys;
    UInt8 rootParamIndex = 0;

	for (auto& shader : shaders) {
        // Set Shader Stages
		if (shader->GetShaderTypeId() == VERTEX_SHADER) {
			m_vertexShader = shader;
		}
		else if (shader->GetShaderTypeId() == GEOMETRY_SHADER) {
            m_geometryShader = shader;
		}
		else if (shader->GetShaderTypeId() == PIXEL_SHADER) {
			m_pixelShader = shader;
		}

		// Create Reflection Data by merging all shader reflections
		auto shaderReflection = shader->GetRawReflection();	
        
		m_reflection.AddShaderReflection(shaderReflection.Get());
	}
}

bool QuantumEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	m_rootSignature = m_reflection.CreateRootSignature(device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, error);
	
    return m_rootSignature != nullptr;
}
