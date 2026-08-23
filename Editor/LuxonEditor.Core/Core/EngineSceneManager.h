#pragma once
#include <EngineAPI.h>
#include <functional>
#include "AssetRegistry.h"

namespace LuxonEditor {
	class __declspec(dllexport) EngineSceneManager {
	public:
		struct EntityEntry {
			boost::uuids::uuid uuid;
			ref<LuxonEngine::GameEntity> entity;
		};

		struct SceneEditor {
			ref<LuxonEngine::Scene> scene;
			std::map<boost::uuids::uuid, EntityEntry> entityMap;
			std::vector<EntityEntry> entityList;
		};

		using EntityListChangedCallback = std::function<void()>;
		using RequestRenderCallback = std::function<void()>;
		using SceneLoadedCallback = std::function<void(ref<LuxonEngine::Scene>)>;
		using RenderSettingChangedCallback = std::function<void(ref<LuxonEngine::Scene>)>;

		// initialScenePath: path to load on startup; empty falls back to default scene.
		void Initialize(const std::string& initialScenePath = {});

		void AddEntity(ref<LuxonEngine::GameEntity> entity);
		void RemoveEntity(const boost::uuids::uuid& uuid);

		void SetHybridSupportEnabled(bool enabled);
		void SetRayTracingSupportEnabled(bool enabled);

		ref<LuxonEngine::Scene> GetCurrentScene() const { return m_currentSceneEditor.scene; }
		ref<LuxonEngine::GameEntity> GetEntityByUUID(const boost::uuids::uuid& uuid) const;
		const std::vector<EntityEntry>& GetEntityList() const { return m_currentSceneEditor.entityList; }
		const std::string& GetCurrentScenePath() const { return m_currentScenePath; }

		void RequestRender();

		void LoadScene(const std::string& path);

		size_t RegisterEntityListChangedCallback(EntityListChangedCallback cb);
		void UnregisterEntityListChangedCallback(size_t id);

		size_t RegisterRequestRenderCallback(RequestRenderCallback cb);
		void UnregisterRequestRenderCallback(size_t id);

		size_t RegisterSceneLoadedCallback(SceneLoadedCallback cb);
		void UnregisterSceneLoadedCallback(size_t id);

		size_t RegisterRenderSettingChangedCallback(RenderSettingChangedCallback cb);
		void UnregisterRenderSettingChangedCallback(size_t id);

		void SaveCurrentScene();
		void CreateScene(const std::string& name, const std::string& path, bool openAfterCreate);
	private:
		void InvokeEntityListChangedCallbacks();
		void InvokeSceneLoadedCallbacks();
		void InvokeRenderSettingChangedCallbacks();

		bool TryLoadSceneFromFile(const std::string& path);
		void SaveScene(const std::string& path, const SceneEditor& sceneEditor);
		void AddEntity(const boost::uuids::uuid& uuid, ref<LuxonEngine::GameEntity> entity, SceneEditor& sceneEditor);
		
		void ReplaceMesh(const ref<LuxonEngine::Mesh>& oldMesh, const ref<LuxonEngine::Mesh>& newMesh);
		SceneEditor CreateDefaultScene();

		std::map<size_t, EntityListChangedCallback> m_entityListChangedCallbacks;
		size_t m_lastCallbackId = 0;

		std::map<size_t, RequestRenderCallback> m_requestRenderCallbacks;
		size_t m_lastRequestRenderCallbackId = 0;

		std::map<size_t, SceneLoadedCallback> m_sceneLoadedCallbacks;
		size_t m_lastSceneLoadedCallbackId = 0;

		std::map <size_t, RenderSettingChangedCallback> m_renderSettingChangedCallbacks;
		size_t m_lastRenderSettingChangedCallbackId = 0;

		std::string m_currentScenePath;
		SceneEditor m_currentSceneEditor;
	};
}