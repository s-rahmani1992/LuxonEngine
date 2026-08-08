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
		using RequestRenderCallback = std::function<void()>;
		using SceneLoadedCallback = std::function<void(ref<LuxonEngine::Scene>)>;

		// initialScenePath: path to load on startup; empty falls back to default scene.
		void Initialize(const std::string& initialScenePath = {});

		void AddEntity(ref<LuxonEngine::GameEntity> entity);
		void RemoveEntity(const boost::uuids::uuid& uuid);

		ref<LuxonEngine::Scene> GetCurrentScene() const { return m_currentScene; }
		ref<LuxonEngine::GameEntity> GetEntityByUUID(const boost::uuids::uuid& uuid) const;
		const std::vector<EntityEntry>& GetEntityList() const { return m_entityList; }
		const std::string& GetCurrentScenePath() const { return m_currentScenePath; }

		void RequestRender();

		void LoadScene(const std::string& path);

		size_t RegisterEntityListChangedCallback(EntityListChangedCallback cb);
		void UnregisterEntityListChangedCallback(size_t id);

		size_t RegisterRequestRenderCallback(RequestRenderCallback cb);
		void UnregisterRequestRenderCallback(size_t id);

		size_t RegisterSceneLoadedCallback(SceneLoadedCallback cb);
		void UnregisterSceneLoadedCallback(size_t id);

		void SaveCurrentScene();
	private:
		void InvokeEntityListChangedCallbacks();
		void InvokeSceneLoadedCallbacks();
		bool TryLoadSceneFromFile(const std::string& path);
		void CreateDefaultScene();

		ref<LuxonEngine::Scene> m_currentScene;
		std::map<boost::uuids::uuid, ref<LuxonEngine::GameEntity>> m_entityMap;
		std::vector<EntityEntry> m_entityList;

		std::map<size_t, EntityListChangedCallback> m_entityListChangedCallbacks;
		size_t m_lastCallbackId = 0;

		std::map<size_t, RequestRenderCallback> m_requestRenderCallbacks;
		size_t m_lastRequestRenderCallbackId = 0;

		std::map<size_t, SceneLoadedCallback> m_sceneLoadedCallbacks;
		size_t m_lastSceneLoadedCallbackId = 0;

		std::string m_currentScenePath;
	};
}