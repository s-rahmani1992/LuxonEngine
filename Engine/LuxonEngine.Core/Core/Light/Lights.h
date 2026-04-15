#pragma once
#include "../Vector3.h"
#include "../Color.h"
#include "../Vector2.h"

namespace QuantumEngine {
	struct Attenuation {
		Float c0 = 0.0f;
		Float c1 = 0.0f;
		Float c2 = 1.0f;
	};

	struct DirectionalLight {
		Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Vector3 direction = Vector3(1.0f, 0.0f, 0.0f);
		float intensity = 1.0f;
	};

	struct PointLight {
		Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
		float intensity = 1.0f;
		Attenuation attenuation;
		float radius = 1.0f;
	};

	struct SceneLightData {
		std::vector<DirectionalLight> directionalLights;
		std::vector<PointLight> pointLights;
	};
}