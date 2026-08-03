#pragma once
#include <EngineAPI.h>
#include <functional>

namespace LuxonEditor {
	class __declspec(dllexport) EngineSceneManager {
	public:
		struct EntityEntry {
			boost::uuids::uuid uuid;
			ref<LuxonEngine::GameEntity> entity;
		};

		using EntityListChangedCallback = std::function<void()>;

		void Initialize();

		void AddEntity(ref<LuxonEngine::GameEntity> entity);
		void RemoveEntity(const boost::uuids::uuid& uuid);

		ref<LuxonEngine::Scene> GetCurrentScene() const { return m_currentScene; }
		ref<LuxonEngine::GameEntity> GetEntityByUUID(const boost::uuids::uuid& uuid) const;
		const std::vector<EntityEntry>& GetEntityList() const { return m_entityList; }

		size_t RegisterEntityListChangedCallback(EntityListChangedCallback cb);
		void UnregisterEntityListChangedCallback(size_t id);
		void SaveCurrentScene();
	private:
		void InvokeEntityListChangedCallbacks();

		ref<LuxonEngine::Scene> m_currentScene;
		std::map<boost::uuids::uuid, ref<LuxonEngine::GameEntity>> m_entityMap;
		std::vector<EntityEntry> m_entityList;

		std::map<size_t, EntityListChangedCallback> m_entityListChangedCallbacks;
		size_t m_lastCallbackId = 0;
		std::string m_currentScenePath;
	};
}