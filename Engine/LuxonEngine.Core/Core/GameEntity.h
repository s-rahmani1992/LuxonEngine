#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
	class Transform;

	namespace Rendering {
		class Renderer;
		class RayTracingComponent;
	}
}

namespace QuantumEngine {
	class GameEntity {
	public:
		GameEntity(const ref<Transform>& transform
			, const ref<Rendering::Renderer>& renderer, const ref<Rendering::RayTracingComponent>& rtComponent)
			:m_transform(transform), m_renderer(renderer), m_rtComponent(rtComponent) { }
	public:
		inline ref<Transform> GetTransform() const { return m_transform; }
		inline ref<Rendering::Renderer> GetRenderer() { return m_renderer; }
		inline ref<Rendering::RayTracingComponent> GetRayTracingComponent() { return m_rtComponent; }
	private:
		ref<Transform> m_transform;
		ref<Rendering::Renderer> m_renderer;
		ref<Rendering::RayTracingComponent> m_rtComponent;
	};
}