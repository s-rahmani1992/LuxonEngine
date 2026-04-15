#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine::Rendering {
	class Shader {
	public:
		Shader(Byte* byteCode, UInt64 codeSize, Int32 shaderTypeId)
			:m_codeSize(codeSize), m_shaderTypeId(shaderTypeId)
		{
			m_byteCode = new Byte[m_codeSize];
			std::memcpy(m_byteCode, byteCode, m_codeSize);
		}

		virtual ~Shader() {
			delete[] m_byteCode;
		}

		inline void* GetByteCode() const { return m_byteCode; }
		inline UInt64 GetCodeSize() const { return m_codeSize; }
		inline Int32 GetShaderTypeId() const { return m_shaderTypeId; }
	protected:
		Byte* m_byteCode;
		UInt64 m_codeSize;
		Int32 m_shaderTypeId;
	};
}