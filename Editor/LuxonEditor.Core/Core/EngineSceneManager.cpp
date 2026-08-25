#include "EngineSceneManager.h"
#include "GuidUtilities.h"
#include "EngineApplication.h"
#include <filesystem>
#include <Core/SerializationStream.h>
#include "SerializationStreamExtensions.h"
#include "AssetRegistry.h"
#include "GuidUtilities.h"

namespace LuxonEditor {

	// Resolves a project-relative path to an absolute path using the project root.
	static std::filesystem::path ResolveAbsolutePath(const std::string& relativePath)
	{
		return std::filesystem::path(EngineApplication::GetProjectPath()) / relativePath;
	}

	void EngineSceneManager::Initialize(const std::string& initialScenePath)
	{
		EngineApplication::GetAssetManager()->RegisterMeshDeletedCallback([this](ref<LuxonEngine::Mesh>& deletedMesh) {
			// Remove any entities that reference the deleted mesh
			ReplaceMesh(deletedMesh, nullptr);
			});

		EngineApplication::GetAssetManager()->RegisterMeshChangedCallback([this](ref<LuxonEngine::Mesh>& oldMesh, const ref<LuxonEngine::Mesh>& newMesh) {
			// Update any entities that reference the old mesh to use the new mesh
			ReplaceMesh(oldMesh, newMesh);
			});

		EngineApplication::GetAssetManager()->RegisterMaterialDeletedCallback([this](ref<LuxonEngine::Rendering::Material>& deletedMaterial) {
			// Remove any entities that reference the deleted material
			for (auto& entry : m_currentSceneEditor.entityList) {

				auto renderer = entry.entity->GetRenderer();

				if (renderer) {
					if(renderer->GetMaterial() == deletedMaterial) {
						renderer->SetMaterial(nullptr);
					}
				}

				auto rtComponent = entry.entity->GetRayTracingComponent();

				if(rtComponent) {
					if(rtComponent->GetRTMaterial() == deletedMaterial) {
						rtComponent->SetRTMaterial(nullptr);
					}
				}
			}
			});

		// Try caller-supplied relative path first
		if (!initialScenePath.empty()) {
			auto absolutePath = ResolveAbsolutePath(initialScenePath);
			if (TryLoadSceneFromFile(absolutePath.string())) {
				m_currentScenePath = initialScenePath;
				InvokeEntityListChangedCallbacks();
				return;
			}
		}

		// Fall back to default relative scene path
		m_currentScenePath = "Assets/DefaultScene.lscene";
		if (TryLoadSceneFromFile(ResolveAbsolutePath(m_currentScenePath).string())) {
			InvokeEntityListChangedCallbacks();
			return;
		}

		m_currentSceneEditor = CreateDefaultScene();
	}

	bool EngineSceneManager::TryLoadSceneFromFile(const std::string& path)
	{
		using namespace LuxonEngine;
		m_currentSceneEditor.scene = std::make_shared<Scene>();
		m_currentSceneEditor.entityMap.clear();
		m_currentSceneEditor.entityList.clear();

		if (!std::filesystem::exists(path))
			return false;

		SerializationStream stream;
		if (!stream.LoadFromFile(path))
			return false;

		auto cameraStream = stream.Object("main-camera");
		auto camTransformStream = cameraStream.Object("transform");
		auto camTransform = DeserializeTransform(camTransformStream);
		auto nearZ = cameraStream.GetFloat("near-z", 0.1f);
		auto farZ = cameraStream.GetFloat("far-z", 100.0f);
		auto fovAngle = cameraStream.GetFloat("fov-angle", 60.0f);
		m_currentSceneEditor.scene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, nearZ, farZ, 16.0f / 9.0f, fovAngle);

		auto lightStream = stream.Object("light-data");

		for (auto& dirLightStream : lightStream.Array("directional-lights"))
			m_currentSceneEditor.scene->lightData.directionalLights.push_back(DeserializeDirectionalLight(dirLightStream));

		for (auto& pointLightStream : lightStream.Array("point-lights"))
			m_currentSceneEditor.scene->lightData.pointLights.push_back(DeserializePointLight(pointLightStream));

		for (auto& entityStream : stream.Array("entities")) {
			auto entity = DeserializeGameEntity(entityStream);
			if (entity) {
				boost::uuids::uuid uuid = entityStream.GetGuid("uuid");

				if(uuid.is_nil())
					uuid = GuidGenerator::GenerateGUID();

				AddEntity(uuid, entity, m_currentSceneEditor);
			}
		}

		for(auto& behaviourStream : stream.Array("behaviours")) {
			auto behaviour = DeserializeBehaviour(behaviourStream);
			if(behaviour) {
				m_currentSceneEditor.scene->behaviours.push_back(behaviour);
			}
		}

		auto rtGlobalGuid = stream.GetGuid("rt-global-material_uuid");
		if(rtGlobalGuid.is_nil() == false)
		{
			m_currentSceneEditor.scene->rtGlobalMaterial = EngineApplication::GetAssetManager()->GetMaterial(rtGlobalGuid);
		}

		stream.GetColor("hybrid-background-color", m_currentSceneEditor.scene->hybridBackgroundColor);

		int supportRT = stream.GetInt("supprt-rt", 0);
		m_currentSceneEditor.scene->canSupportRayTracing = (supportRT != 0);

		int supportHybrid = stream.GetInt("support-hybrid", 0);
		m_currentSceneEditor.scene->canSupportHybridRendering = (supportHybrid != 0);
		
		return true;
	}

	void EngineSceneManager::SaveScene(const std::string& path, const SceneEditor& sceneEditor)
	{
		LuxonEngine::SerializationStream stream;

		LuxonEngine::SerializationStream cameraStream;
		LuxonEngine::SerializationStream cameraTransformStream;
		SerializeTransform(cameraTransformStream, sceneEditor.scene->mainCamera->GetTransform());
		cameraStream.SetObject("transform", cameraTransformStream);
		auto perspectiveCamera = std::dynamic_pointer_cast<LuxonEngine::PerspectiveCamera>(sceneEditor.scene->mainCamera);
		cameraStream.SetFloat("near-z", perspectiveCamera->GetNearZ());
		cameraStream.SetFloat("far-z", perspectiveCamera->GetFarZ());
		cameraStream.SetFloat("fov-angle", perspectiveCamera->GetFovAngle());
		stream.SetObject("main-camera", cameraStream);

		LuxonEngine::SerializationStream lightDataStream;
		std::vector<LuxonEngine::SerializationStream> directionalLightsArray;
		for (const auto& light : sceneEditor.scene->lightData.directionalLights)
		{
			LuxonEngine::SerializationStream lightStream;
			SerializeDirectionalLight(lightStream, light);
			directionalLightsArray.push_back(lightStream);
		}
		lightDataStream.SetArray("directional-lights", directionalLightsArray);

		std::vector<LuxonEngine::SerializationStream> pointLightsArray;
		for (const auto& light : sceneEditor.scene->lightData.pointLights)
		{
			LuxonEngine::SerializationStream lightStream;
			SerializePointLight(lightStream, light);
			pointLightsArray.push_back(lightStream);
		}
		lightDataStream.SetArray("point-lights", pointLightsArray);

		stream.SetObject("light-data", lightDataStream);

		std::vector<LuxonEngine::SerializationStream> entitiesArray;
		for (const auto& entry : sceneEditor.entityList)
		{
			LuxonEngine::SerializationStream entityStream;
			SerializeGameEntity(entityStream, entry.entity);
			entityStream.SetGuid("uuid", entry.uuid);
			entitiesArray.push_back(entityStream);
		}
		stream.SetArray("entities", entitiesArray);

		std::vector<LuxonEngine::SerializationStream> behavioursArray;

		for (const auto& behaviour : sceneEditor.scene->behaviours)
		{
			LuxonEngine::SerializationStream behaviourStream;
			SerializeBehaviour(behaviourStream, behaviour);
			behavioursArray.push_back(behaviourStream);
		}
		stream.SetArray("behaviours", behavioursArray);

		if(sceneEditor.scene->rtGlobalMaterial)
		{
			stream.SetGuid("rt-global-material_uuid", EngineApplication::GetAssetManager()->GetMaterialEntry(sceneEditor.scene->rtGlobalMaterial)->guid);
		}

		stream.SetColor("hybrid-background-color", sceneEditor.scene->hybridBackgroundColor);

		stream.SetInt("supprt-rt", sceneEditor.scene->canSupportRayTracing);
		stream.SetInt("support-hybrid", sceneEditor.scene->canSupportHybridRendering);
		
		stream.SaveToFile(ResolveAbsolutePath(path).string());
	}

	void EngineSceneManager::AddEntity(const boost::uuids::uuid& uuid, ref<LuxonEngine::GameEntity> entity, SceneEditor& sceneEditor)
	{
		sceneEditor.entityMap[uuid] = { uuid, entity };
		sceneEditor.scene->entities.push_back(entity);
		sceneEditor.entityList.push_back({ uuid, entity });
	}

	void EngineSceneManager::ReplaceMesh(const ref<LuxonEngine::Mesh>& oldMesh, const ref<LuxonEngine::Mesh>& newMesh)
	{
		bool sceneNeedsUpdate = false;
		for (const auto& entry : m_currentSceneEditor.entityList) {
			auto renderer = entry.entity->GetRenderer();

			if (renderer != nullptr) {
				auto mesh = renderer->GetMesh();

				if (mesh == oldMesh) {
					sceneNeedsUpdate = true;
					renderer->SetMesh(newMesh);
				}
			}

			auto rtComponent = entry.entity->GetRayTracingComponent();

			if (rtComponent != nullptr)
			{
				auto rtMesh = rtComponent->GetMesh();
				if (rtMesh == oldMesh)
				{
					sceneNeedsUpdate = true;
					rtComponent->SetMesh(newMesh);
				}
			}

			if (sceneNeedsUpdate) {
				RequestRender();
			}
		}
	}

	LuxonEditor::EngineSceneManager::SceneEditor EngineSceneManager::CreateDefaultScene()
	{
		using namespace LuxonEngine;
		auto assetManager = EngineApplication::GetAssetManager();

		SceneEditor editorScene;
		editorScene.scene = std::make_shared<Scene>();

		auto defaultMaterial = assetManager->GetMaterial(LuxonEditor::GuidGenerator::GenerateGUIDFromString("222d330a-5081-4e42-8b2e-0b09ecbbaaf5"));
		
		auto sphereTransform = std::make_shared<Transform>(Vector3(0.0f, 1.5f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		auto sphereMesh = assetManager->GetMesh(LuxonEditor::GuidGenerator::GenerateGUIDFromString("585aa516-59c6-4fb4-a893-e4d56f65431a"));
		auto sphereRenderer = std::make_shared<Rendering::MeshRenderer>(sphereMesh, defaultMaterial);
		auto sphereEntity = std::make_shared<GameEntity>(sphereTransform, sphereRenderer, nullptr);
		sphereEntity->SetName("Sphere");

		auto planeTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f), Vector3(0.0f, 1.0f, 0.0f), 45);
		auto planeMesh = assetManager->GetMesh(LuxonEditor::GuidGenerator::GenerateGUIDFromString("837f517a-9689-4c3f-88a9-0042120f3abd"));
		auto planeRenderer = std::make_shared<Rendering::MeshRenderer>(planeMesh, defaultMaterial);
		auto planeEntity = std::make_shared<GameEntity>(planeTransform, planeRenderer, nullptr);
		planeEntity->SetName("Plane");

		auto camTransform = std::make_shared<Transform>(Vector3(0.0f, 1.0f, -5.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		editorScene.scene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, 0.1f, 100.0f, 16.0f / 9.0f, 60.0f);

		editorScene.scene->lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(-1.0f, -1.0f, -1.0f),
			.intensity = 1.0f
			});

		AddEntity(GuidGenerator::GenerateGUID(), sphereEntity, editorScene);
		AddEntity(GuidGenerator::GenerateGUID(), planeEntity, editorScene);

		return editorScene;
	}

	void EngineSceneManager::LoadScene(const std::string& relativePath)
	{
		if (!TryLoadSceneFromFile(ResolveAbsolutePath(relativePath).string()))
			return;

		m_currentScenePath = relativePath;
		InvokeEntityListChangedCallbacks();
		InvokeSceneLoadedCallbacks();
		RequestRender();
	}

	void EngineSceneManager::AddEntity(ref<LuxonEngine::GameEntity> entity)
	{
		boost::uuids::uuid uuid = GuidGenerator::GenerateGUID();
		AddEntity(uuid, entity, m_currentSceneEditor);
		InvokeEntityListChangedCallbacks();
		RequestRender();
	}

	void EngineSceneManager::RemoveEntity(const boost::uuids::uuid& uuid)
	{
		auto mapIt = m_currentSceneEditor.entityMap.find(uuid);
		if (mapIt == m_currentSceneEditor.entityMap.end())
			return;

		ref<LuxonEngine::GameEntity> entity = mapIt->second.entity;
		m_currentSceneEditor.entityMap.erase(mapIt);
		auto& sceneEntities = m_currentSceneEditor.scene->entities;
		sceneEntities.erase(
			std::remove(sceneEntities.begin(), sceneEntities.end(), entity),
			sceneEntities.end());

		for (auto it = m_currentSceneEditor.entityList.begin(); it != m_currentSceneEditor.entityList.end(); ++it)
		{
			if (it->uuid == uuid)
			{
				m_currentSceneEditor.entityList.erase(it);
				break;
			}
		}

		InvokeEntityListChangedCallbacks();
		RequestRender();
	}

	void EngineSceneManager::SetHybridSupportEnabled(bool enabled)
	{
		if (m_currentSceneEditor.scene->canSupportHybridRendering != enabled) {
			m_currentSceneEditor.scene->canSupportHybridRendering = enabled;
			InvokeRenderSettingChangedCallbacks();
		}
	}

	void EngineSceneManager::SetRayTracingSupportEnabled(bool enabled)
	{
		if (m_currentSceneEditor.scene->canSupportRayTracing != enabled) {
			m_currentSceneEditor.scene->canSupportRayTracing = enabled;
			InvokeRenderSettingChangedCallbacks();
		}
	}

	ref<LuxonEngine::GameEntity> EngineSceneManager::GetEntityByUUID(const boost::uuids::uuid& uuid) const
	{
		auto it = m_currentSceneEditor.entityMap.find(uuid);
		if (it != m_currentSceneEditor.entityMap.end())
			return it->second.entity;
		return nullptr;
	}

	void EngineSceneManager::RequestRender()
	{
		for (auto& kv : m_requestRenderCallbacks)
			kv.second();
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

	size_t EngineSceneManager::RegisterRequestRenderCallback(RequestRenderCallback cb)
	{
		auto id = ++m_lastRequestRenderCallbackId;
		m_requestRenderCallbacks[id] = cb;
		return id;
	}

	void EngineSceneManager::UnregisterRequestRenderCallback(size_t id)
	{
		m_requestRenderCallbacks.erase(id);
	}

	size_t EngineSceneManager::RegisterSceneLoadedCallback(SceneLoadedCallback cb)
	{
		size_t id = ++m_lastSceneLoadedCallbackId;
		m_sceneLoadedCallbacks[id] = cb;
		return id;
	}

	void EngineSceneManager::UnregisterSceneLoadedCallback(size_t id)
	{
		m_sceneLoadedCallbacks.erase(id);
	}

	size_t EngineSceneManager::RegisterRenderSettingChangedCallback(RenderSettingChangedCallback cb)
	{
		auto id = ++m_lastRenderSettingChangedCallbackId;
		m_renderSettingChangedCallbacks[id] = cb;
		return id;
	}

	void EngineSceneManager::UnregisterRenderSettingChangedCallback(size_t id)
	{
		m_renderSettingChangedCallbacks.erase(id);
	}

	void EngineSceneManager::SaveCurrentScene()
	{
		SaveScene(m_currentScenePath, m_currentSceneEditor);
	}

	void EngineSceneManager::CreateScene(const std::string& name, const std::string& path, bool openAfterCreate)
	{
		//auto tempSceneEditor = m_currentSceneEditor;
		std::filesystem::path absolutePath = ResolveAbsolutePath(path);
		std::filesystem::create_directories(absolutePath.parent_path());
		auto newScene = CreateDefaultScene();

		if (openAfterCreate) {
			SaveCurrentScene();

			m_currentScenePath = path;
			m_currentSceneEditor = newScene;
			InvokeEntityListChangedCallbacks();
			InvokeSceneLoadedCallbacks();
			RequestRender();
			SaveCurrentScene();
		}
		else {
			SaveScene(path, newScene);
		}
	}

	void EngineSceneManager::RemoveBehaviour(const ref<LuxonEngine::Behaviour>& behaviour)
	{
		auto& behaviours = m_currentSceneEditor.scene->behaviours;
		auto it = std::find(behaviours.begin(), behaviours.end(), behaviour);
		if (it != behaviours.end()) {
			behaviours.erase(it);
		}
	}

	ref<LuxonEngine::Transform> EngineSceneManager::GetTransformOfEntity(const boost::uuids::uuid& uuid) const
	{
		auto entity = GetEntityByUUID(uuid);
		return entity ? entity->GetTransform() : nullptr;
	}

	boost::uuids::uuid EngineSceneManager::GetEntityGUIDFromTransform(const ref<LuxonEngine::Transform>& transform) const
	{
		auto it = std::find_if(m_currentSceneEditor.entityList.begin(), m_currentSceneEditor.entityList.end(),
			[&transform](const EntityEntry& entry) {
				return entry.entity->GetTransform() == transform;
			});

		return (it != m_currentSceneEditor.entityList.end()) ? it->uuid : boost::uuids::uuid();
	}

	void EngineSceneManager::InvokeEntityListChangedCallbacks()
	{
		for (auto& kv : m_entityListChangedCallbacks)
			kv.second();
	}

	void EngineSceneManager::InvokeSceneLoadedCallbacks()
	{
		for (auto& kv : m_sceneLoadedCallbacks)
			kv.second(m_currentSceneEditor.scene);
	}

	void EngineSceneManager::InvokeRenderSettingChangedCallbacks()
	{
		for (auto& kv : m_renderSettingChangedCallbacks)
			kv.second(m_currentSceneEditor.scene);
	}
}