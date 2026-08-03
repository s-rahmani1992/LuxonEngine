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
}