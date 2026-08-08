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
		m_currentScene = std::make_shared<LuxonEngine::Scene>();

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

		CreateDefaultScene();
	}

	bool EngineSceneManager::TryLoadSceneFromFile(const std::string& path)
	{
		using namespace LuxonEngine;

		if (!std::filesystem::exists(path))
			return false;

		SerializationStream stream;
		if (!stream.LoadFromFile(path))
			return false;

		m_currentScene = std::make_shared<Scene>();
		m_entityMap.clear();
		m_entityList.clear();

		auto cameraStream = stream.Object("main-camera");
		auto camTransformStream = cameraStream.Object("transform");
		auto camTransform = DeserializeTransform(camTransformStream);
		auto nearZ = cameraStream.GetFloat("near-z", 0.1f);
		auto farZ = cameraStream.GetFloat("far-z", 100.0f);
		auto fovAngle = cameraStream.GetFloat("fov-angle", 60.0f);
		m_currentScene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, nearZ, farZ, 16.0f / 9.0f, fovAngle);

		auto lightStream = stream.Object("light-data");

		for (auto& dirLightStream : lightStream.Array("directional-lights"))
			m_currentScene->lightData.directionalLights.push_back(DeserializeDirectionalLight(dirLightStream));

		for (auto& pointLightStream : lightStream.Array("point-lights"))
			m_currentScene->lightData.pointLights.push_back(DeserializePointLight(pointLightStream));

		for (auto& entityStream : stream.Array("entities")) {
			auto entity = DeserializeGameEntity(entityStream);
			if (entity) {
				boost::uuids::uuid uuid = GuidGenerator::GenerateGUID();
				m_entityMap[uuid] = entity;
				m_currentScene->entities.push_back(entity);
				m_entityList.push_back({ uuid, entity });
			}
		}

		return true;
	}

	void EngineSceneManager::CreateDefaultScene()
	{
		using namespace LuxonEngine;

		m_currentScene = std::make_shared<Scene>();
		m_entityMap.clear();
		m_entityList.clear();

		auto transform1 = std::make_shared<Transform>(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		auto entity1 = std::make_shared<GameEntity>(transform1, nullptr, nullptr);
		entity1->SetName("Default Entity");

		auto transform2 = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 45);
		auto entity2 = std::make_shared<GameEntity>(transform2, nullptr, nullptr);
		entity2->SetName("Second Entity");

		auto camTransform = std::make_shared<Transform>(Vector3(0.0f, 1.0f, -5.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		m_currentScene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, 0.1f, 100.0f, 16.0f / 9.0f, 60.0f);

		m_currentScene->lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(-1.0f, -1.0f, -1.0f),
			.intensity = 1.0f
			});

		m_currentScene->lightData.pointLights.push_back(PointLight{
			.color = Color(1.0f, 0.0f, 0.0f, 1.0f),
			.position = Vector3(-2.0f, 2.0f, 2.0f),
			.intensity = 5.0f,
			.attenuation = Attenuation{.c0 = 1.0f, .c1 = 0.1f, .c2 = 0.01f },
			.radius = 10.0f
			});

		m_currentScene->lightData.pointLights.push_back(PointLight{
			.color = Color(1.0f, 0.0f, 1.0f, 1.0f),
			.position = Vector3(2.0f, 2.0f, 2.0f),
			.intensity = 5.0f,
			.attenuation = Attenuation{.c0 = 1.0f, .c1 = 0.1f, .c2 = 0.01f },
			.radius = 10.0f
			});

		AddEntity(entity1);
		AddEntity(entity2);
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
		m_entityMap[uuid] = entity;
		m_currentScene->entities.push_back(entity);
		m_entityList.push_back({ uuid, entity });
		InvokeEntityListChangedCallbacks();
		RequestRender();
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
		RequestRender();
	}

	ref<LuxonEngine::GameEntity> EngineSceneManager::GetEntityByUUID(const boost::uuids::uuid& uuid) const
	{
		auto it = m_entityMap.find(uuid);
		if (it != m_entityMap.end())
			return it->second;
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
		LuxonEngine::SerializationStream stream;

		LuxonEngine::SerializationStream cameraStream;
		LuxonEngine::SerializationStream cameraTransformStream;
		SerializeTransform(cameraTransformStream, m_currentScene->mainCamera->GetTransform());
		cameraStream.SetObject("transform", cameraTransformStream);
		auto perspectiveCamera = std::dynamic_pointer_cast<LuxonEngine::PerspectiveCamera>(m_currentScene->mainCamera);
		cameraStream.SetFloat("near-z", perspectiveCamera->GetNearZ());
		cameraStream.SetFloat("far-z", perspectiveCamera->GetFarZ());
		cameraStream.SetFloat("fov-angle", perspectiveCamera->GetFovAngle());
		stream.SetObject("main-camera", cameraStream);

		LuxonEngine::SerializationStream lightDataStream;
		std::vector<LuxonEngine::SerializationStream> directionalLightsArray;
		for (const auto& light : m_currentScene->lightData.directionalLights)
		{
			LuxonEngine::SerializationStream lightStream;
			SerializeDirectionalLight(lightStream, light);
			directionalLightsArray.push_back(lightStream);
		}
		lightDataStream.SetArray("directional-lights", directionalLightsArray);

		std::vector<LuxonEngine::SerializationStream> pointLightsArray;
		for (const auto& light : m_currentScene->lightData.pointLights)
		{
			LuxonEngine::SerializationStream lightStream;
			SerializePointLight(lightStream, light);
			pointLightsArray.push_back(lightStream);
		}
		lightDataStream.SetArray("point-lights", pointLightsArray);

		stream.SetObject("light-data", lightDataStream);

		std::vector<LuxonEngine::SerializationStream> entitiesArray;
		for (const auto& entry : m_entityList)
		{
			LuxonEngine::SerializationStream entityStream;
			SerializeGameEntity(entityStream, entry.entity);
			entitiesArray.push_back(entityStream);
		}
		stream.SetArray("entities", entitiesArray);
		stream.SaveToFile(ResolveAbsolutePath(m_currentScenePath).string());
	}

	void EngineSceneManager::InvokeEntityListChangedCallbacks()
	{
		for (auto& kv : m_entityListChangedCallbacks)
			kv.second();
	}

	void EngineSceneManager::InvokeSceneLoadedCallbacks()
	{
		for (auto& kv : m_sceneLoadedCallbacks)
			kv.second(m_currentScene);
	}
}