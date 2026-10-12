#pragma once
#include "../BasicTypes.h"
#include "RendererType.h"

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

		virtual RendererTypeId GetTypeId() const = 0;

	public:
		ref<Mesh> GetMesh() const { return m_mesh; }
		ref<Material> GetMaterial() const { return material; }

		void SetMesh(const ref<Mesh>& mesh) { m_mesh = mesh; }
		void SetMaterial(const ref<Material>& material) { this->material = material; }
	private:
		ref<Mesh> m_mesh;
		ref<Material> material;
	};

	template<typename T>
	T* RendererCast(Renderer* renderer)
	{
		if (renderer == nullptr || renderer->GetTypeId() != T::StaticTypeId())
			return nullptr;

		return static_cast<T*>(renderer);
	}
}
