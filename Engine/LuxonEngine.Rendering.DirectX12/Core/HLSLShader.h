#pragma once
#include "Rendering/Shader.h"
#include <vector>
#include <map>
#include <string>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	enum DX12_Shader_Type { //TODO If possible, convert to enum class
		VERTEX_SHADER = 0,
		PIXEL_SHADER = 1,
		GEOMETRY_SHADER = 2,
	};

	class HLSLShader : public Shader {
	public:
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType);
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12ShaderReflection>& shaderReflection);
		
		inline ComPtr<ID3D12ShaderReflection> GetRawReflection() { return m_rawReflection; }

	private:

		DX12_Shader_Type m_shaderType;
		std::wstring m_empty;
		ComPtr<ID3D12ShaderReflection> m_rawReflection;
	};
}