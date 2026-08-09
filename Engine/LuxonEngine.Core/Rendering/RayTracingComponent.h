#pragma once
#include "../BasicTypes.h"

namespace LuxonEngine {
	class Mesh;
}

namespace LuxonEngine::Rendering {
	class Material;

	class RayTracingComponent {
	public:
		RayTracingComponent(const ref<Mesh>& mesh, const ref<Material>& rtMaterial)
			:m_mesh(mesh), m_rtMaterial(rtMaterial) {
		}
	public:
		ref<Mesh> GetMesh() const { return m_mesh; }
		ref<Material> GetRTMaterial() const { return m_rtMaterial; }

		void SetMesh(const ref<Mesh>& mesh) { m_mesh = mesh; }
		void SetRTMaterial(const ref<Material>& rtMaterial) { m_rtMaterial = rtMaterial; }
	private:
		ref<Mesh> m_mesh;
		ref<Material> m_rtMaterial;
	};
}