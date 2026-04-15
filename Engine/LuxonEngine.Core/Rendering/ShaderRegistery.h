#pragma once
#include "../BasicTypes.h"
#include <string>

namespace QuantumEngine::Rendering {
	class ShaderProgram;
	class Shader;

	class ShaderRegistery {
	public:

		/// <summary>
		/// abstract method for compiling file into a complete shader program
		/// </summary>
		/// <param name="fileName">name of the file</param>
		/// <param name="error">contains error message if compilation fails</param>
		/// <returns></returns>
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) = 0;
		
		/// <summary>
		/// Registers shader program with name in order to be retrieved later. used for internal shaders
		/// </summary>
		/// <param name="name"></param>
		/// <param name="program"></param>
		/// <param name="isRT"></param>
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) = 0;
	};
}
