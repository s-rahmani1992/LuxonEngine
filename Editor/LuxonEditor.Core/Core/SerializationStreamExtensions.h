#pragma once
#include <EngineAPI.h>
#include <Core/SerializationStream.h>

namespace LuxonEditor {
	ref<LuxonEngine::Transform> DeserializeTransform(LuxonEngine::SerializationStream& stream);
	void SerializeTransform(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Transform>& transform);

	ref<LuxonEngine::Rendering::Renderer> DeserializeRenderer(LuxonEngine::SerializationStream& stream);
	void SerializeRenderer(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Rendering::Renderer>& renderer);

	ref<LuxonEngine::GameEntity> DeserializeGameEntity(LuxonEngine::SerializationStream& stream);
	void SerializeGameEntity(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::GameEntity>& entity);

	LuxonEngine::DirectionalLight DeserializeDirectionalLight(LuxonEngine::SerializationStream& stream);
	void SerializeDirectionalLight(LuxonEngine::SerializationStream& stream, const LuxonEngine::DirectionalLight& light);

	LuxonEngine::PointLight DeserializePointLight(LuxonEngine::SerializationStream& stream);
	void SerializePointLight(LuxonEngine::SerializationStream& stream, const LuxonEngine::PointLight& light);

	ref<LuxonEngine::Rendering::RayTracingComponent> DeserializeRayTracingComponent(LuxonEngine::SerializationStream& stream);
	void SerializeRayTracingComponent(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Rendering::RayTracingComponent>& rtComponent);
}