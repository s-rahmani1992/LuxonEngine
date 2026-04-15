#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering {
	class Material;

	class RayTracingComponent {
	public:
		RayTracingComponent(const ref<Mesh>& mesh, const ref<Material>& rtMaterial)
			:m_mesh(mesh), m_rtMaterial(rtMaterial) {
		}
	public:
		ref<Mesh> GetMesh() const { return m_mesh; }
		ref<Material> GetRTMaterial() const { return m_rtMaterial; }
	private:
		ref<Mesh> m_mesh;
		ref<Material> m_rtMaterial;
	};
}