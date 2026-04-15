#pragma once

#include "../BasicTypes.h"
#include "Renderer.h"

namespace LuxonEngine {
	class Mesh;
}

namespace LuxonEngine::Rendering {
	class Material;

	class GBufferRTReflectionRenderer : public Renderer
	{
	public:
		GBufferRTReflectionRenderer(const ref<Mesh>& mesh, const ref<Material>& material)
			: Renderer(mesh, material) {
		}
	};
}