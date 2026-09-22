#include "ShaderRegistery.h"
#include "ShaderProgram.h"

LuxonEngine::Rendering::ShaderRegistery::ShaderRegistery(ShaderCompiler* shaderCompiler)
	: m_shaderCompiler(shaderCompiler)
{
}

LuxonEngine::Rendering::ShaderRegistery::~ShaderRegistery() = default;

void LuxonEngine::Rendering::ShaderRegistery::AddShaderProgram(const std::string& identifierName, ShaderProgram* program)
{
	m_namedPrograms[identifierName] = program;
}
LuxonEngine::Rendering::ShaderProgram* LuxonEngine::Rendering::ShaderRegistery::GetShaderProgram(const std::string& identifierName) const
{
	auto it = m_namedPrograms.find(identifierName);
	if (it == m_namedPrograms.end())
		return nullptr;

	return it->second;
}
