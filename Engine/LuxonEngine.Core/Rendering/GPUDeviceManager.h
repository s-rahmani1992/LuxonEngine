#pragma once
#include "../BasicTypes.h"
#include"../Platform/GraphicWindow.h"

namespace LuxonEngine::Rendering {
	using namespace Platform;

	class GraphicContext;
	class GPUAssetManager;
	class ShaderCompiler;
	class ShaderRegistery;
	class MaterialFactory;

	/// <summary>
	/// an abstract class representing GPU Adapter device
	/// </summary>
	class GPUDeviceManager {
	public:
		/// <summary>
		/// Initializes the device manager and returns true if successful
		/// </summary>
		/// <returns></returns>
		virtual bool Initialize() = 0;

		/// <summary>
		/// Creates a hybrid graphic renderer for window
		/// </summary>
		/// <param name="window">target window object</param>
		/// <returns></returns>
		virtual ref<GraphicContext> CreateHybridContextForWindows(ref<GraphicWindow>& window) = 0;
		
		/// <summary>
		/// Creates a full ray tracing graphic renderer for window
		/// </summary>
		/// <param name="window">target window object</param>
		/// <returns></returns>
		virtual ref<GraphicContext> CreateRayTracingContextForWindows(ref<GraphicWindow>& window) = 0;
		
		virtual ref<GraphicContext> CreateEditorContext(ref<LuxonEngine::Platform::GraphicWindow>& window) { return nullptr; };

		/// <summary>
		/// Creates Asset Manager for uploading and managing GPU assets
		/// </summary>
		/// <returns></returns>
		virtual ref<GPUAssetManager> CreateAssetManager() = 0;

		/// <summary>
		/// Creates Shader Compiler for managing shaders and shader programs
		/// </summary>
		/// <returns></returns>
		virtual ref<ShaderCompiler> GetShaderCompiler() = 0;

		/// <summary>
		/// Creates Material Factory for creating materials from shader programs
		/// </summary>
		/// <returns></returns>
		virtual ref<MaterialFactory> CreateMaterialFactory() = 0;

		/// <summary>
		/// Sets the shader registery used by the graphic contexts to retrieve internal shader programs.
		/// the device manager does not take ownership of the registery
		/// </summary>
		/// <param name="shaderRegistery">shader registery</param>
		void SetShaderRegistery(ShaderRegistery* shaderRegistery) { m_shaderRegistery = shaderRegistery; }

	protected:
		ShaderRegistery* m_shaderRegistery = nullptr;
	};
}