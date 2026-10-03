#include "pch.h"
#include "HLSLRasterizationProgram.h"
#include "HLSLShader.h"
#include <set>

LuxonEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram::HLSLRasterizationProgram(const std::vector<HLSLShaderData>& shaders)
{
    std::set<std::string> keys;
    UInt8 rootParamIndex = 0;

	for (auto& shader : shaders) {
        // Set Shader Stages
		if (shader.shaderType == D3D12_SHVER_VERTEX_SHADER) {
			m_vertexShader = shader.byteCode;
		}
		else if (shader.shaderType == D3D12_SHVER_GEOMETRY_SHADER) {
            m_geometryShader = shader.byteCode;
		}
		else if (shader.shaderType == D3D12_SHVER_PIXEL_SHADER) {
			m_pixelShader = shader.byteCode;
		}

		// Create Reflection Data by merging all shader reflections
		auto shaderReflection = shader.reflection;	
        
		m_reflection.AddShaderReflection(shaderReflection.Get());
	}
}

bool LuxonEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	m_rootSignature = m_reflection.CreateRootSignature(device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, error);
	
    return m_rootSignature != nullptr;
}
