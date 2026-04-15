#pragma once
#include "../BasicTypes.h"
#include "GameEntity.h"
#include "Camera/Camera.h"
#include "Light/Lights.h"
#include "Texture2D.h"
#include "../Rendering/Material.h"
#include "Behaviour.h"
#include <vector>

namespace QuantumEngine {
	namespace Rendering {
		class Material;
	}

	class Scene {
	public:
		ref<Camera> mainCamera;
		SceneLightData lightData;
		std::vector<ref<GameEntity>> entities;
		ref<Rendering::Material> rtGlobalMaterial;
		std::vector<ref<Behaviour>> behaviours;
	};
}