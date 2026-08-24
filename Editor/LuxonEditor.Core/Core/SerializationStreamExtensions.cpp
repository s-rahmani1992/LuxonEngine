#include "SerializationStreamExtensions.h"
#include "EngineApplication.h"
#include "AssetRegistry.h"
#include "../Behaviours/MaterialValueModifier.h"

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
			case 1:
			{
				LuxonEngine::Vector3 point0, point1, point2;
				stream.GetVector3("point0", point0);
				stream.GetVector3("point1", point1);
				stream.GetVector3("point2", point2);
				float width = stream.GetFloat("width", 1.0f);
				int segments = stream.GetInt("segments", 1);
				auto materialGuid = stream.GetGuid("material-guid");
				auto material = EngineApplication::GetAssetManager()->GetMaterial(materialGuid);
				return std::make_shared<LuxonEngine::Rendering::SplineRenderer>(material, std::vector<LuxonEngine::Vector3>{point0, point1, point2}, width, segments);
			}
			case 2:
				{
				auto materialGuid = stream.GetGuid("material-guid");
				auto material = EngineApplication::GetAssetManager()->GetMaterial(materialGuid);
				auto meshGuid = stream.GetGuid("mesh-guid");
				auto mesh = EngineApplication::GetAssetManager()->GetMesh(meshGuid);
				return std::make_shared<LuxonEngine::Rendering::GBufferRTReflectionRenderer>(mesh, material);
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
		else if( auto splineRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::SplineRenderer>(renderer)) {
			stream.SetInt("renderer-type", 1);
			auto material = splineRenderer->GetMaterial();
			if (material) {
				auto materialEntry = EngineApplication::GetAssetManager()->GetMaterialEntry(material);
				stream.SetGuid("material-guid", materialEntry->guid);
			}
			auto& curve = splineRenderer->GetCurve();
			stream.SetVector3("point0", curve.m_point1);
			stream.SetVector3("point1", curve.m_point2);
			stream.SetVector3("point2", curve.m_point3);
			stream.SetFloat("width", splineRenderer->GetWidth());
			stream.SetInt("segments", splineRenderer->GetSegments());
		}
		else if(auto gBufferRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::GBufferRTReflectionRenderer>(renderer)) {
			stream.SetInt("renderer-type", 2);
			auto material = gBufferRenderer->GetMaterial();
			auto mesh = gBufferRenderer->GetMesh();
			if (material) {
				auto materialEntry = EngineApplication::GetAssetManager()->GetMaterialEntry(material);
				stream.SetGuid("material-guid", materialEntry->guid);
			}
			if (mesh) {
				auto meshEntry = EngineApplication::GetAssetManager()->GetMeshEntry(mesh);
				stream.SetGuid("mesh-guid", meshEntry->guid);
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

	ref<LuxonEngine::Behaviour> DeserializeBehaviour(LuxonEngine::SerializationStream& stream)
	{
		int type = stream.GetInt("behaviour-type", -1);

		switch (type) {
			case 0: // MaterialValueModifier
			{
				auto materialGuid = stream.GetGuid("material-guid");
				auto material = EngineApplication::GetAssetManager()->GetMaterial(materialGuid);
				std::string fieldName;
				stream.GetString("field-name", fieldName);
				float speed = stream.GetFloat("speed", 0.0f);
				float minValue = stream.GetFloat("min-value", 0.0f);
				float maxValue = stream.GetFloat("max-value", 1.0f);
				return std::make_shared<MaterialValueModifier>(material, fieldName, speed, minValue, maxValue);
			}
			default:
				return nullptr;
		}
	}

	void SerializeBehaviour(LuxonEngine::SerializationStream& stream, const ref<LuxonEngine::Behaviour>& behaviour)
	{
		if (!behaviour) {
			return;
		}
		if (auto materialValueModifier = std::dynamic_pointer_cast<MaterialValueModifier>(behaviour)) {
			stream.SetInt("behaviour-type", 0);
			auto material = materialValueModifier->GetMaterial();
			if (material) {
				auto materialEntry = EngineApplication::GetAssetManager()->GetMaterialEntry(material);
				stream.SetGuid("material-guid", materialEntry->guid);
			}
			stream.SetString("field-name", materialValueModifier->GetFieldName());
			stream.SetFloat("speed", materialValueModifier->GetSpeed());
			stream.SetFloat("min-value", materialValueModifier->GetMinValue());
			stream.SetFloat("max-value", materialValueModifier->GetMaxValue());
		}
		else {
			stream.SetInt("behaviour-type", -1);
		}
	}
}