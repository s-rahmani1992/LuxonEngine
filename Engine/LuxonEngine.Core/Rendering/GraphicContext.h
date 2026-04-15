#pragma once
#include "../BasicTypes.h"
#include "../Core/Matrix4.h"
#include "../Core/Vector3.h"
#include "../Core/Camera/Camera.h"
#include <vector>

namespace QuantumEngine {
	class Mesh;
	class GameEntity;
	struct SceneLightData;
	class Scene;
}

namespace QuantumEngine::Rendering {
	class GPUAssetManager;
	class ShaderProgram;
	class ShaderRegistery;

	class GraphicContext {
	public:
		virtual void Render() = 0;
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& assetManager) = 0;
		virtual void RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery) = 0;
		virtual bool PrepareScene(const ref<Scene>& scene) = 0;
	protected:
		struct CameraGPU {
		public:
			Matrix4 projectionMatrix;
			Matrix4 inverseProjectionMatrix;
			Matrix4 viewMatrix;
			Vector3 position;
		};
		
	};
}