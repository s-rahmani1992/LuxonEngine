#pragma once
#include "../BasicTypes.h"
#include "Renderer.h"

namespace LuxonEngine {
	class Mesh;
}

namespace LuxonEngine::Rendering {
	class Material;

	class MeshRenderer : public Renderer 
	{
	public:
		MeshRenderer(const ref<Mesh>& mesh, const ref<Material>& material)
			: Renderer(mesh, material) { }
	};
}