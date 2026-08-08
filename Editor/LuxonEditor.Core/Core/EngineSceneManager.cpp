#include "EngineSceneManager.h"
#include "GuidUtilities.h"
#include "EngineApplication.h"
#include <filesystem>
#include <Core/SerializationStream.h>
#include "SerializationStreamExtensions.h"

namespace LuxonEditor {

	void EngineSceneManager::Initialize()
	{
		using namespace LuxonEngine;

		std::filesystem::path projectPath = EngineApplication::GetProjectPath();

		m_currentScenePath = (projectPath / "Assets" / "DefaultScene.lscene").string();
		m_currentScene = std::make_shared<Scene>();

		if(std::filesystem::exists(m_currentScenePath)) {
			LuxonEngine::SerializationStream stream;
			if(stream.LoadFromFile(m_currentScenePath)) {
				auto cameraStream = stream.Object("main-camera");
				auto camTransformStream = cameraStream.Object("transform");
				auto camTransform = DeserializeTransform(camTransformStream);
				auto nearZ = cameraStream.GetFloat("near-z", 0.1f);
				auto farZ = cameraStream.GetFloat("far-z", 100.0f);
				auto fovAngle = cameraStream.GetFloat("fov-angle", 60.0f);
				m_currentScene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, nearZ, farZ, 16.0f / 9.0f, fovAngle);

				auto lightStram = stream.Object("light-data");

				std::vector<SerializationStream> directionalLightsArray = lightStram.Array("directional-lights");
				for(auto& dirLightStream : directionalLightsArray) {
					auto dirLight = DeserializeDirectionalLight(dirLightStream);
					m_currentScene->lightData.directionalLights.push_back(dirLight);
				}

				std::vector<SerializationStream> pointLightsArray = lightStram.Array("point-lights");
				for(auto& pointLightStream : pointLightsArray) {
					auto pointLight = DeserializePointLight(pointLightStream);
					m_currentScene->lightData.pointLights.push_back(pointLight);
				}

				auto entitiesArray = stream.Array("entities");
				for(auto& entityStream : entitiesArray) {
					auto entity = DeserializeGameEntity(entityStream);
					if(entity) {
						AddEntity(entity);
					}
				}
				InvokeEntityListChangedCallbacks();
				return;
			}
		}

		auto transform1 = std::make_shared<Transform>(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		auto entity1 = std::make_shared<LuxonEngine::GameEntity>(transform1, nullptr, nullptr);
		entity1->SetName("Default Entity");

		auto transform2 = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 45);
		auto entity2 = std::make_shared<LuxonEngine::GameEntity>(transform2, nullptr, nullptr);
		entity2->SetName("Second Entity");

		auto camTransform = std::make_shared<Transform>(Vector3(0.0f, 1.0f, -5.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
		m_currentScene->mainCamera = std::make_shared<PerspectiveCamera>(camTransform, 0.1f, 100.0f, 16.0f / 9.0f, 60.0f);

		m_currentScene->lightData.directionalLights.push_back(LuxonEngine::DirectionalLight{ 
			.color = LuxonEngine::Color(1.0f, 1.0f, 1.0f, 1.0f), 
			.direction = LuxonEngine::Vector3(-1.0f, -1.0f, -1.0f), 
			.intensity = 1.0f
			}
		);

		m_currentScene->lightData.pointLights.push_back(LuxonEngine::PointLight{
			.color = LuxonEngine::Color(1.0f, 0.0f, 0.0f, 1.0f),
			.position = LuxonEngine::Vector3(-2.0f, 2.0f, 2.0f),
			.intensity = 5.0f,
			.attenuation = LuxonEngine::Attenuation{ .c0 = 1.0f, .c1 = 0.1f, .c2 = 0.01f },
			.radius = 10.0f
			}
		);

		m_currentScene->lightData.pointLights.push_back(LuxonEngine::PointLight{
			.color = LuxonEngine::Color(1.0f, 0.0f, 1.0f, 1.0f),
			.position = LuxonEngine::Vector3(2.0f, 2.0f, 2.0f),
			.intensity = 5.0f,
			.attenuation = LuxonEngine::Attenuation{ .c0 = 1.0f, .c1 = 0.1f, .c2 = 0.01f },
			.radius = 10.0f
			}
		);

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
		for(auto& kv : m_requestRenderCallbacks)
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
		stream.SaveToFile(m_currentScenePath);
	}

	void EngineSceneManager::InvokeEntityListChangedCallbacks()
	{
		for (auto& kv : m_entityListChangedCallbacks)
			kv.second();
	}
}