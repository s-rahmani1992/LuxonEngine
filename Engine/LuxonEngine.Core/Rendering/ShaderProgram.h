#pragma once
#include "../BasicTypes.h"
#include <initializer_list>
#include <vector>
#include "Shader.h"

namespace QuantumEngine::Rendering {

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

		/// <summary>
		/// Gets the shader of specified enum type (VERTEX_SHADER, PIXEL_SHADER, etc.)
		/// </summary>
		/// <param name="index"></param>
		/// <returns></returns>
		ref<Shader> GetShader(Int32 index) {
			auto it = std::find_if(m_shaders.begin(), m_shaders.end(), [index](ref<Shader>& item) {
				return item->GetShaderTypeId() == index;
			});

			if (it != m_shaders.end())
				return *it;
			
			return nullptr;
		}
	protected:
		std::vector<ref<Shader>> m_shaders;
	};
}