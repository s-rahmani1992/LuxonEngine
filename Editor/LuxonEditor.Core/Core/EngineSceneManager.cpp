#include "EngineSceneManager.h"
#include "GuidUtilities.h"
#include "EngineApplication.h"
#include <filesystem>
#include <Core/SerializationStream.h>
#include "SerializationStreamExtensions.h"

namespace LuxonEditor {

	// Resolves a project-relative path to an absolute path using the project root.
	static std::filesystem::path ResolveAbsolutePath(const std::string& relativePath)
	{
		return std::filesystem::path(EngineApplication::GetProjectPath()) / relativePath;
	}

	void EngineSceneManager::Initialize(const std::string& initialScenePath)
	{
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
		stream.SaveToFile(ResolveAbsolutePath(path).string());
	}

	void EngineSceneManager::AddEntity(const boost::uuids::uuid& uuid, ref<LuxonEngine::GameEntity> entity, SceneEditor& sceneEditor)
	{
		sceneEditor.entityMap[uuid] = { uuid, entity };
		sceneEditor.scene->entities.push_back(entity);
		sceneEditor.entityList.push_back({ uuid, entity });
	}

	LuxonEditor::EngineSceneManager::SceneEditor EngineSceneManager::CreateDefaultScene()
	{
		using namespace LuxonEngine;

		SceneEditor editorScene;
		editorScene.scene = std::make_shared<Scene>();

		auto transform1 = std::make_shared<Transform>(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		auto entity1 = std::make_shared<GameEntity>(transform1, nullptr, nullptr);
		entity1->SetName("Sphere");

		auto transform2 = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 45);
		auto entity2 = std::make_shared<GameEntity>(transform2, nullptr, nullptr);
		entity2->SetName("Second Entity");

		auto camTransform = std::make_shared<Transform>(Vector3(0.0f, 1.0f, -5.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		editorScene.scene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, 0.1f, 100.0f, 16.0f / 9.0f, 60.0f);

		editorScene.scene->lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(-1.0f, -1.0f, -1.0f),
			.intensity = 1.0f
			});

		editorScene.scene->lightData.pointLights.push_back(PointLight{
			.color = Color(1.0f, 0.0f, 0.0f, 1.0f),
			.position = Vector3(-2.0f, 2.0f, 2.0f),
			.intensity = 5.0f,
			.attenuation = Attenuation{.c0 = 1.0f, .c1 = 0.1f, .c2 = 0.01f },
			.radius = 10.0f
			});

		editorScene.scene->lightData.pointLights.push_back(PointLight{
			.color = Color(1.0f, 0.0f, 1.0f, 1.0f),
			.position = Vector3(2.0f, 2.0f, 2.0f),
			.intensity = 5.0f,
			.attenuation = Attenuation{.c0 = 1.0f, .c1 = 0.1f, .c2 = 0.01f },
			.radius = 10.0f
			});

		AddEntity(GuidGenerator::GenerateGUID(), entity1, editorScene);
		AddEntity(GuidGenerator::GenerateGUID(), entity2, editorScene);

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
}