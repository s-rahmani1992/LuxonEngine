#include "SerializationStreamExtensions.h"
#include "EngineApplication.h"
#include "AssetRegistry.h"

namespace LuxonEditor {
	ref<LuxonEngine::Transform> DeserializeTransform(LuxonEngine::SerializationStream& stream)
	{
		auto position = LuxonEngine::Vector3(0.0f);

		if (stream.GetVector3("position", position) == false) {
			return nullptr;
		}

		auto axis = LuxonEngine::Vector3(0.0f, 1.0f, 0.0f);

		if (stream.GetVector3("axis", axis) == false) {
			return nullptr;
		}

		auto angleDeg = stream.GetFloat("angleDeg", 0.0f);

		auto scale = LuxonEngine::Vector3(1.0f);

		if (stream.GetVector3("scale", scale) == false) {
			return nullptr;
		}

		return std::make_shared<LuxonEngine::Transform>(position, scale, axis, angleDeg);
	}

	void SerializeTransform(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Transform>& transform)
	{
		if (!transform) {
			return;
		}
		stream.SetVector3("position", transform->Position());
		stream.SetVector3("axis", transform->RotationAxis());
		stream.SetFloat("angleDeg", transform->GetAngle());
		stream.SetVector3("scale", transform->Scale());
	}

	ref<LuxonEngine::Rendering::Renderer> DeserializeRenderer(LuxonEngine::SerializationStream& stream)
	{
		int rendererType = stream.GetInt("renderer-type", -1);

		switch(rendererType) {
			case 0: // MeshRenderer
			{
				auto meshGuid = stream.GetGuid("mesh-guid");
				auto materialGuid = stream.GetGuid("material-guid");
				auto mesh = EngineApplication::GetAssetManager()->GetMesh(meshGuid);
				auto material = EngineApplication::GetAssetManager()->GetMaterial(materialGuid);
				return std::make_shared<LuxonEngine::Rendering::MeshRenderer>(mesh, material);
			}
			default:
				return nullptr;
		}
	}

	void SerializeRenderer(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Rendering::Renderer>& renderer)
	{
		if (!renderer) {
			return;
		}
		if (auto meshRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::MeshRenderer>(renderer)) {
			stream.SetInt("renderer-type", 0);
			auto mesh = meshRenderer->GetMesh();
			auto material = meshRenderer->GetMaterial();
			if (mesh) {
				auto meshEntry = EngineApplication::GetAssetManager()->GetMeshEntry(mesh);
				stream.SetGuid("mesh-guid", meshEntry->guid);
			}
			if (material) {
				auto materialEntry = EngineApplication::GetAssetManager()->GetMaterialEntry(material);
				stream.SetGuid("material-guid", materialEntry->guid);
			}
		}
		else {
			stream.SetInt("renderer-type", -1);
		}
	}

	ref<LuxonEngine::GameEntity> DeserializeGameEntity(LuxonEngine::SerializationStream& stream)
	{
		auto transformStream = stream.Object("transform");
		auto transform = DeserializeTransform(transformStream);

		auto rendererStream = stream.Object("renderer");
		auto renderer = DeserializeRenderer(rendererStream);

		std::string name;
		stream.GetString("name", name);

		auto rtComponentStream = stream.Object("ray-tracing-component");
		ref<LuxonEngine::Rendering::RayTracingComponent> rtComponent = nullptr;

		if (rtComponentStream.IsEmpty() == false) {
			rtComponent = DeserializeRayTracingComponent(rtComponentStream);
		}

		auto entity = std::make_shared<LuxonEngine::GameEntity>(transform, renderer, rtComponent);
		entity->SetName(name);
		return entity;
	}

	void SerializeGameEntity(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::GameEntity>& entity)
	{
		if (!entity) {
			return;
		}
		auto transform = entity->GetTransform();
		auto transformStream = LuxonEngine::SerializationStream();
		SerializeTransform(transformStream, transform);

		auto renderer = entity->GetRenderer();
		auto rendererStream = LuxonEngine::SerializationStream();
		SerializeRenderer(rendererStream, renderer);

		stream.SetObject("transform", transformStream);
		stream.SetObject("renderer", rendererStream);
		stream.SetString("name", entity->GetName());

		auto rtComponent = entity->GetRayTracingComponent();
		if(rtComponent) {
			auto rtComponentStream = LuxonEngine::SerializationStream();
			SerializeRayTracingComponent(rtComponentStream, rtComponent);
			stream.SetObject("ray-tracing-component", rtComponentStream);
		}
	}

	LuxonEngine::DirectionalLight DeserializeDirectionalLight(LuxonEngine::SerializationStream& stream)
	{
		LuxonEngine::DirectionalLight directionalLight;
		stream.GetColor("color", directionalLight.color);
		stream.GetVector3("direction", directionalLight.direction);
		directionalLight.intensity = stream.GetFloat("intensity", 1.0f);
		return directionalLight;
	}

	void SerializeDirectionalLight(LuxonEngine::SerializationStream& stream, const LuxonEngine::DirectionalLight& light)
	{
		stream.SetColor("color", light.color);
		stream.SetVector3("direction", light.direction);
		stream.SetFloat("intensity", light.intensity);
	}

	LuxonEngine::PointLight DeserializePointLight(LuxonEngine::SerializationStream& stream)
	{
		LuxonEngine::PointLight pointLight;
		stream.GetColor("color", pointLight.color);
		stream.GetVector3("position", pointLight.position);
		pointLight.intensity = stream.GetFloat("intensity", 1.0f);
		pointLight.attenuation.c0 = stream.GetFloat("attenuation-c0", 0.0f);
		pointLight.attenuation.c1 = stream.GetFloat("attenuation-c1", 0.0f);
		pointLight.attenuation.c2 = stream.GetFloat("attenuation-c2", 1.0f);
		pointLight.radius = stream.GetFloat("radius", 1.0f);
		return pointLight;
	}

	void SerializePointLight(LuxonEngine::SerializationStream& stream, const LuxonEngine::PointLight& light)
	{
		stream.SetColor("color", light.color);
		stream.SetVector3("position", light.position);
		stream.SetFloat("intensity", light.intensity);
		stream.SetFloat("attenuation-c0", light.attenuation.c0);
		stream.SetFloat("attenuation-c1", light.attenuation.c1);
		stream.SetFloat("attenuation-c2", light.attenuation.c2);
		stream.SetFloat("radius", light.radius);
	}

	ref<LuxonEngine::Rendering::RayTracingComponent> DeserializeRayTracingComponent(LuxonEngine::SerializationStream& stream)
	{
		auto meshGuid = stream.GetGuid("mesh-guid");
		auto materialGuid = stream.GetGuid("material-guid");
		auto mesh = EngineApplication::GetAssetManager()->GetMesh(meshGuid);
		auto material = EngineApplication::GetAssetManager()->GetMaterial(materialGuid);
		return std::make_shared<LuxonEngine::Rendering::RayTracingComponent>(mesh, material);
	}

	void SerializeRayTracingComponent(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Rendering::RayTracingComponent>& rtComponent)
	{
		if (!rtComponent) {
			return;
		}
		auto mesh = rtComponent->GetMesh();
		auto material = rtComponent->GetRTMaterial();
		if (mesh) {
			auto meshEntry = EngineApplication::GetAssetManager()->GetMeshEntry(mesh);
			stream.SetGuid("mesh-guid", meshEntry->guid);
		}
		if (material) {
			auto materialEntry = EngineApplication::GetAssetManager()->GetMaterialEntry(material);
			stream.SetGuid("material-guid", materialEntry->guid);
		}
	}
}