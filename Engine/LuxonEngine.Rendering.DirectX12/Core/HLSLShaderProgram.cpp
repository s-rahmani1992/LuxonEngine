#include "pch.h"
#include "HLSLShaderProgram.h"

UInt32 LuxonEngine::Rendering::DX12::RootParameterLayout::AddParameter(const std::string& name, bool isConstantData)
{
	UInt32 rootParameterIndex = m_rootParameterCount;

	if (isConstantData)
		m_constantDataRootIndex = rootParameterIndex;

	m_rootParameterIndices[name] = rootParameterIndex;
	m_rootParameterCount++;

	return rootParameterIndex;
}

UInt32 LuxonEngine::Rendering::DX12::RootParameterLayout::GetRootParameterIndex(const std::string& name) const
{
	auto it = m_rootParameterIndices.find(name);

	if (it == m_rootParameterIndices.end())
		return ShaderVariableReflection::InvalidIndex;

	return it->second;
}
