#pragma once
#include "../export.h"
#include <string>
#include <map>

namespace LuxonEngine::Rendering {
	class ShaderCompiler;
	class ShaderProgram;

	class LUXON_CORE_API ShaderRegistery {
	public:
		ShaderRegistery(ShaderCompiler* shaderCompiler);
		virtual ~ShaderRegistery();

		ShaderRegistery(const ShaderRegistery&) = delete;
		ShaderRegistery& operator=(const ShaderRegistery&) = delete;

		void AddShaderProgram(const std::string& identifierName, ShaderProgram* program);
		ShaderProgram* GetShaderProgram(const std::string& identifierName) const;
		ShaderCompiler* GetShaderCompiler() const { return m_shaderCompiler; }

	protected:
		ShaderCompiler* m_shaderCompiler;
		std::map<std::string, ShaderProgram*> m_namedPrograms;
	};
}
