#include "pch.h"
#include "HLSLRasterizationProgram.h"
#include "HLSLShader.h"
#include <set>
#include "../Core/HLSLVariableReflection.h"

LuxonEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram::HLSLRasterizationProgram(const std::vector<HLSLShaderData>& shaders)
{
    std::set<std::string> keys;
    UInt8 rootParamIndex = 0;

	for (auto& shader : shaders) {
		auto shaderReflection = shader.reflection.Get();

        // Set Shader Stages
		if (shader.shaderType == D3D12_SHVER_VERTEX_SHADER) {
			m_vertexShader = shader.byteCode;
			FillInputAssemblyReflection(m_inputAssemblyReflection, shaderReflection);
		}
		else if (shader.shaderType == D3D12_SHVER_GEOMETRY_SHADER) {
            m_geometryShader = shader.byteCode;
		}
		else if (shader.shaderType == D3D12_SHVER_PIXEL_SHADER) {
			m_pixelShader = shader.byteCode;
			FillRenderTargetReflection(m_renderTargetReflection, shaderReflection);
		}

		m_reflection.AddShaderReflection(shaderReflection);
		AddShaderReflection(m_variableReflection, shaderReflection);
	}
}

bool LuxonEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error)
{
	m_rootSignature = m_reflection.CreateRootSignature(device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, error);
	
    return m_rootSignature != nullptr;
}
