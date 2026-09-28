#pragma once

#include "Rendering/GraphicContext.h"
#include "BasicTypes.h"
#include <vector>
#include <map>
#include "DX12LightManager.h"

#define GLOBAL_HIT_GROUP_NAME L"GlobalHit"

namespace LuxonEngine {
	class GameEntity;
	class Transform;

	namespace Platform {
		class GraphicWindow;
	}

	namespace Rendering {
		class MeshRenderer;
		class GBufferRTReflectionRenderer;
		class Material;
	}
}

namespace LuxonEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;
	class DX12CommandExecuter;
	class DX12MeshController;
	class DX12AssetManager;
	class HLSLMaterial;
	class HLSLShaderProgram;
	class DX12ShaderCompiler;
	struct EntityGBufferData;
	class DX12PipelineFactory;

	namespace RayTracing {
		class RTAccelarationStructure;
	}

	namespace MeshShading {
		class DX12MeshPipelineModule;
	}

	struct DX12MeshShadingPipelineData {
		ref<MeshShading::DX12MeshPipelineModule> pipeline;
		UInt32 taskGroupCount; // group count of the largest mesh drawn by the pipeline
	};

	struct DX12EntityGPUData {
	public:
		ref<GameEntity> gameEntity;
		ComPtr<ID3D12Resource2> transformResource;
		ComPtr<ID3D12DescriptorHeap> transformHeap;
	};

	struct DX12RayTracingGPUData {
	public:
		ref<LuxonEngine::Rendering::DX12::DX12MeshController> meshController;
		ref<Material> material;
		ComPtr<ID3D12Resource2> transformResource;
		ref<Transform> transform;
	};

	struct TransformGPU {
	public:
		Matrix4 modelMatrix;
		Matrix4 rotationMatrix;
		Matrix4 modelViewMatrix;
	};

	class DX12GraphicContext : public GraphicContext
	{
	public:
		DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<LuxonEngine::Platform::GraphicWindow>& window, const ref<DX12AssetManager>& assetManager);
		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) = 0;
		virtual void Flush() override;
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& mesh) override;
		virtual void RegisterShaderRegistery(const ref<ShaderCompiler>& shaderRegistery) override;
		virtual void Detach() override;
	protected:
		bool InitializeCommandObjects(const ComPtr<ID3D12Device10>& device);
		bool InitializeSwapChain(const ComPtr<IDXGIFactory7>& factory);
		bool InitializeCamera(const ref<Camera>& camera);
		bool InitializeLight(const SceneLightData& lights);
		void InitializeEntityGPUData(const std::vector<ref<GameEntity>>& gameEntities);
		void UploadTexturesAndMeshes(const ref<Scene>& scene);
		void UpdateDataHeaps();

		/// <summary>
		/// Creates one mesh pipeline per material of the spike mesh renderers of the entities and fills their descriptors and constants.
		/// the entity GPU data, camera and light must be initialized before
		/// </summary>
		std::vector<DX12MeshShadingPipelineData> CreateSpikeMeshPipelines(DX12PipelineFactory& pipelineFactory);

		/// <summary>
		/// Draws the mesh pipelines. they bind their own descriptor heaps, so the heap of the other pipelines must be set again after them
		/// </summary>
		void DispatchMeshShadingPipelines(const std::vector<DX12MeshShadingPipelineData>& pipelines);

		ref<LuxonEngine::Platform::GraphicWindow> m_window;
		ComPtr<ID3D12Device10> m_device;
		ref<DX12CommandExecuter> m_commandExecuter; 
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12GraphicsCommandList7> m_commandList;

		ref<DX12AssetManager> m_assetManager;
		ref<DX12ShaderCompiler> m_shaderRegistery;

		std::vector<DX12EntityGPUData> m_entityGPUData;

		// Swap Chain And Render Targets
		UInt8 m_bufferCount;
		ComPtr<IDXGISwapChain4> m_swapChain;
		std::vector<ComPtr<ID3D12Resource2>> m_renderBuffers;
		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;

		// Camera
		ref<Camera> m_camera;
		CameraGPU m_camData;
		ComPtr<ID3D12Resource2> m_cameraBuffer;
		ComPtr<ID3D12DescriptorHeap> m_cameraHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE m_cameraHandle;

		// Light
		D3D12_GPU_DESCRIPTOR_HANDLE m_lightHandle;
		DX12LightManager m_lightManager;

	private:
		TransformGPU m_transformData;
	};
}
