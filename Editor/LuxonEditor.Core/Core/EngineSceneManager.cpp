#include "../Core/EngineSceneManager.h"
#include "../Core/GuidUtilities.h"

namespace LuxonEditor {

	void EngineSceneManager::Initialize()
	{
		using namespace LuxonEngine;

		auto transform1 = std::make_shared<Transform>(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		auto entity1 = std::make_shared<LuxonEngine::GameEntity>(transform1, nullptr, nullptr);
		entity1->SetName("Default Entity");

		auto transform2 = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 45);
		auto entity2 = std::make_shared<LuxonEngine::GameEntity>(transform2, nullptr, nullptr);
		entity2->SetName("Second Entity");

		m_currentScene = std::make_shared<Scene>();
		AddEntity(entity1);
		AddEntity(entity2);
	}

	void EngineSceneManager::AddEntity(ref<LuxonEngine::GameEntity> entity)
	{
		boost::uuids::uuid uuid = GuidGenerator::GenerateGUID();
		m_entityMap[uuid] = entity;
		m_currentScene->entities.push_back(entity);
		m_entityList.push_back({ uuid, entity });
		InvokeEntityListChangedCallbacks();
	}

	void EngineSceneManager::RemoveEntity(const boost::uuids::uuid& uuid)
	{
		auto mapIt = m_entityMap.find(uuid);
		if (mapIt == m_entityMap.end())
			return;

		ref<LuxonEngine::GameEntity> entity = mapIt->second;
		m_entityMap.erase(mapIt);

		auto& sceneEntities = m_currentScene->entities;
		sceneEntities.erase(
			std::remove(sceneEntities.begin(), sceneEntities.end(), entity),
			sceneEntities.end());

		for (auto it = m_entityList.begin(); it != m_entityList.end(); ++it)
		{
			if (it->uuid == uuid)
			{
				m_entityList.erase(it);
				break;
			}
		}

		InvokeEntityListChangedCallbacks();
	}

	ref<LuxonEngine::GameEntity> EngineSceneManager::GetEntityByUUID(const boost::uuids::uuid& uuid) const
	{
		auto it = m_entityMap.find(uuid);
		if (it != m_entityMap.end())
			return it->second;
		return nullptr;
	}

	size_t EngineSceneManager::RegisterEntityListChangedCallback(EntityListChangedCallback cb)
	{
		size_t id = ++m_lastCallbackId;
		m_entityListChangedCallbacks[id] = cb;
		return id;
	}

	void EngineSceneManager::UnregisterEntityListChangedCallback(size_t id)
	{
		m_entityListChangedCallbacks.erase(id);
	}

	void EngineSceneManager::InvokeEntityListChangedCallbacks()
	{
		for (auto& kv : m_entityListChangedCallbacks)
			kv.second();
	}
}