#pragma once
#include "../BasicTypes.h"
#include "Renderer.h"

namespace LuxonEngine {
	class Mesh;
	class Texture2D;
}

namespace LuxonEngine::Rendering {
	class Material;

	class SurfaceInstanceRenderer : public Renderer
	{
	public:
		SurfaceInstanceRenderer(const ref<Mesh>& instanceMesh, const ref<Material>& material, const ref<Texture2D>& maskTexture = nullptr, float instanceScale = 1.0f, float density = 1.0f)
			: Renderer(material), m_instanceMesh(instanceMesh), m_maskTexture(maskTexture), m_instanceScale(instanceScale), m_density(density) { }

		inline ref<Mesh> GetInstanceMesh() const { return m_instanceMesh; }

		inline void SetInstanceMesh(const ref<Mesh>& instanceMesh) { m_instanceMesh = instanceMesh; }

		inline ref<Texture2D> GetMaskTexture() const { return m_maskTexture; }

		inline void SetMaskTexture(const ref<Texture2D>& maskTexture) { m_maskTexture = maskTexture; }

		inline float GetInstanceScale() const { return m_instanceScale; }

		inline void SetInstanceScale(float instanceScale) { m_instanceScale = instanceScale; }

		inline float GetDensity() const { return m_density; }

		inline void SetDensity(float density) { m_density = density; }

	private:
		ref<Mesh> m_instanceMesh;
		ref<Texture2D> m_maskTexture;
		float m_instanceScale = 1.0f;
		float m_density = 1.0f;
	};
}
