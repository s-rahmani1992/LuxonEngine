#pragma once
#include "../BasicTypes.h"
#include <initializer_list>
#include <vector>
#include "Shader.h"
#include "ShaderReflection.h"

namespace LuxonEngine::Rendering {
	enum class ShaderProgramType {
		Rasterization = 0,
		RayTracing = 1,
		Compute = 2,
		Mesh = 3,
	};

	/// <summary>
	/// base class for integration of shaders making up a complete shader program pipeline
	/// </summary>
	class ShaderProgram {
	public:
		ShaderProgram() = default;
		ShaderProgram(const std::initializer_list<ref<Shader>>& shaders) 
			:m_shaders(shaders)
		{

		}

		virtual ~ShaderProgram(){}

		virtual ShaderProgramType GetType() = 0;

		inline const ShaderVariableReflection& GetVariableReflection() const { return m_variableReflection; }
	
	protected:
		std::vector<ref<Shader>> m_shaders;
		ShaderVariableReflection m_variableReflection;
	};
}