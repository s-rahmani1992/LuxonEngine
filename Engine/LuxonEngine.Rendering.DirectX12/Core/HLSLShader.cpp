#include "pch.h"
#include "HLSLShader.h"
#include "StringUtilities.h"
#include <Core/Matrix4.h>

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType)
	:Shader(byteCode, codeLength, shaderType), m_shaderType(shaderType)
{

}

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12ShaderReflection>& shaderReflection)
	:HLSLShader(byteCode, codeLength, shaderType)
{
	m_rawReflection = shaderReflection;
}
