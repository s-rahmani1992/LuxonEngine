#pragma once
#include <BasicTypes.h>
#include <string>

namespace QuantumEngine {
	class Scene;

	namespace Rendering {
		class GPUDeviceManager;
		class GPUAssetManager;
		class ShaderRegistery;
		class MaterialFactory;
	}

	namespace Platform {
		class GraphicWindow;
	}
}

using namespace QuantumEngine;
namespace Render = QuantumEngine::Rendering;
namespace Platform = QuantumEngine::Platform;

class SceneBuilder
{
public:
	static ref<Scene> BuildSimpleLightScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr);
	static ref<Scene> BuildReflectionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr);
	static ref<Scene> BuildShadowScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr);
	static ref<Scene> BuildRefractionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr);
	static ref<Scene> BuildComplexScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr);
};
