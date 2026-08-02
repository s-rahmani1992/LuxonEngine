#pragma once
#include "../BasicTypes.h"
#include <string>

namespace LuxonEngine {
	class Mesh;
	class Transform;

	namespace Rendering {
		class Renderer;
		class RayTracingComponent;
	}
}

namespace LuxonEngine {
	class GameEntity {
	public:
		GameEntity(const ref<Transform>& transform
			, const ref<Rendering::Renderer>& renderer, const ref<Rendering::RayTracingComponent>& rtComponent)
			:m_transform(transform), m_renderer(renderer), m_rtComponent(rtComponent), m_name("new_entity") { }

		void SetName(const std::string& name) { m_name = name; }
	public:
		inline std::string GetName() const { return m_name; }
		inline ref<Transform> GetTransform() const { return m_transform; }
		inline ref<Rendering::Renderer> GetRenderer() { return m_renderer; }
		inline ref<Rendering::RayTracingComponent> GetRayTracingComponent() { return m_rtComponent; }
		inline void SetRenderer(const ref<Rendering::Renderer>& renderer) { m_renderer = renderer; }
	private:
		ref<Transform> m_transform;
		ref<Rendering::Renderer> m_renderer;
		ref<Rendering::RayTracingComponent> m_rtComponent;
		std::string m_name;
	};
}