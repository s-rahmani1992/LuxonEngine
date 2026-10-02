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
		SurfaceInstanceRenderer(const ref<Mesh>& nearMesh, const ref<Mesh>& farMesh, const ref<Material>& material, const ref<Texture2D>& maskTexture = nullptr,
			float instanceScale = 1.0f, float density = 1.0f, float nearDistance = 5.0f)
			: Renderer(material), m_nearMesh(nearMesh), m_farMesh(farMesh), m_nearDistance(nearDistance), m_maskTexture(maskTexture), m_instanceScale(instanceScale), m_density(density) { }

		inline ref<Mesh> GetNearMesh() const { return m_nearMesh; }

		inline void SetNearMesh(const ref<Mesh>& nearMesh) { m_nearMesh = nearMesh; }

		inline ref<Mesh> GetFarMesh() const { return m_farMesh; }

		inline void SetFarMesh(const ref<Mesh>& farMesh) { m_farMesh = farMesh; }

		inline float GetNearDistance() const { return m_nearDistance; }

		inline void SetNearDistance(float nearDistance) { m_nearDistance = nearDistance; }

		inline ref<Texture2D> GetMaskTexture() const { return m_maskTexture; }

		inline void SetMaskTexture(const ref<Texture2D>& maskTexture) { m_maskTexture = maskTexture; }

		inline float GetInstanceScale() const { return m_instanceScale; }

		inline void SetInstanceScale(float instanceScale) { m_instanceScale = instanceScale; }

		inline float GetDensity() const { return m_density; }

		inline void SetDensity(float density) { m_density = density; }

	private:
		ref<Mesh> m_nearMesh;
		ref<Mesh> m_farMesh;
		float m_nearDistance = 5.0f;
		ref<Texture2D> m_maskTexture;
		float m_instanceScale = 1.0f;
		float m_density = 1.0f;
	};
}
