#pragma once

#include "../BasicTypes.h"

namespace QuantumEngine::Rendering {
	class ShaderProgram;
	class Material;

	/// <summary>
	/// factory class for creating materials from shader programs
	/// </summary>
	class MaterialFactory {
	public:
		/// <summary>
		/// Creates a new material from a shader program
		/// </summary>
		/// <param name="program"></param>
		/// <returns></returns>
		virtual ref<Material> CreateMaterial(const ref<ShaderProgram>& program) = 0;
	};
}