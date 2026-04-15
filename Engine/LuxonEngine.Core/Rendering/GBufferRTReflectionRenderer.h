#pragma once

#include "../BasicTypes.h"
#include "Renderer.h"

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering {
	class Material;

	class GBufferRTReflectionRenderer : public Renderer
	{
	public:
		GBufferRTReflectionRenderer(const ref<Mesh>& mesh, const ref<Material>& material)
			: Renderer(mesh, material) {
		}
	};
}