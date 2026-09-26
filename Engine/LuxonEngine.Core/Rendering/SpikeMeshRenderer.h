#pragma once
#include "../BasicTypes.h"
#include "Renderer.h"

namespace LuxonEngine {
	class Mesh;
}

namespace LuxonEngine::Rendering {
	class Material;

	class SpikeMeshRenderer : public Renderer
	{
	public:
		SpikeMeshRenderer(const ref<Mesh>& mesh, const ref<Material>& material, float spikeHeight = 1.0f)
			: Renderer(mesh, material), m_spikeHeight(spikeHeight) { }

		inline float GetSpikeHeight() const { return m_spikeHeight; }

		inline void SetSpikeHeight(float spikeHeight) { m_spikeHeight = spikeHeight; }

	private:
		float m_spikeHeight = 1.0f;
	};
}
