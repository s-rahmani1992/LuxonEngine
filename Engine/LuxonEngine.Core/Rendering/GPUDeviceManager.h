#pragma once
#include "../BasicTypes.h"
#include"../Platform/GraphicWindow.h"

namespace QuantumEngine::Rendering {
	using namespace Platform;

	class GraphicContext;
	class GPUAssetManager;
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
		
		/// <summary>
		/// Creates Asset Manager for uploading and managing GPU assets
		/// </summary>
		/// <returns></returns>
		virtual ref<GPUAssetManager> CreateAssetManager() = 0;

		/// <summary>
		/// Creates Shader Registery for managing shaders and shader programs
		/// </summary>
		/// <returns></returns>
		virtual ref<ShaderRegistery> CreateShaderRegistery() = 0;

		/// <summary>
		/// Creates Material Factory for creating materials from shader programs
		/// </summary>
		/// <returns></returns>
		virtual ref<MaterialFactory> CreateMaterialFactory() = 0;
	};
}