#pragma once

namespace LuxonEngine {
	class Mesh;
}

namespace LuxonEngine::Rendering {
	class Material;

	class Renderer {
	public:
		Renderer(const ref<Material>& material) 
			:material(material) { }

		Renderer(const ref<Mesh>& mesh, const ref<Material>& material)
			:m_mesh(mesh), material(material) { }
		virtual ~Renderer() = default;

	public:
		ref<Mesh> GetMesh() const { return m_mesh; }
		ref<Material> GetMaterial() const { return material; }

	private:
		ref<Mesh> m_mesh;
		ref<Material> material;
	};
}
