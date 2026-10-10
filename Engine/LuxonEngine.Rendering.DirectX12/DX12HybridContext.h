#pragma once
#include "DX12GraphicContext.h"
#include "Core/DX12Buffer.h"
#include "Core/DX12RenderTexture.h"
#include "Core/DX12DepthTexture.h"
#include "Core/DX12RenderPass.h"

namespace LuxonEngine::Rendering {
	class SplineRenderer;
	class ShaderRegistery;
	class ShaderProgram;
}

namespace LuxonEngine::Rendering::DX12 {
	namespace RayTracing {
		class DX12RayTracingPipelineModule;
	}

	namespace Rasterization {
		class DX12RasterizationPipelineModule;
	}

	namespace Compute {
		class DX12ComputePipelineModule;
	}

	class DX12PipelineFactory;
	class DX12GPUResourceManager;

	struct EntityGBufferData {
	public:
		ref<GBufferRTReflectionRenderer> renderer;
		GameEntity* entity;
		D3D12_CPU_DESCRIPTOR_HANDLE transformCpuHandle;
	};

	// render targets of the first stage of the reflection renderer. the index of every array is position, normal, mask
	struct GBufferResources {
	public:
		static constexpr UInt32 TargetCount = 3;
		static constexpr UInt32 Position = 0;
		static constexpr UInt32 Normal = 1;
		static constexpr UInt32 Mask = 2;

		float clearColors[TargetCount][4] = { { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
		DXGI_FORMAT formats[TargetCount] = { DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R8_UINT };
		Ptr<DX12RenderTexture> renderTextures[TargetCount] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[TargetCount] = {};
		Ptr<DX12DepthTexture> depthTexture;
	};

	class DX12HybridContext : public DX12GraphicContext
	{
	public:
		DX12HybridContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<LuxonEngine::Platform::GraphicWindow>& window, const ref<DX12AssetManager>& assetManager,
			const ref<DX12PipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery)
			: DX12GraphicContext(bufferCount, m_commandExecuter, window, assetManager), m_pipelineFactory(pipelineFactory), m_shaderProgramRegistery(shaderRegistery) {
		}

		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		bool InitializeDepthBuffer();
		bool InitializeMainRenderPass();
		void InitializePipelines();
		bool InitializeGBuffer();
		void InitializeRayTracingStage();

		std::vector<ref<MeshShading::DX12MeshPipelineModule>> CreateSurfaceInstancePipelines();

		/// <summary>
		/// Creates one rasterization pipeline per material of the mesh renderers and g buffer renderers of the entities
		/// </summary>
		void CreateRasterizationPipelines();

		/// <summary>
		/// Creates the pipelines of the spline renderers: a compute module per spline that generates the vertices and one rasterization module per material that draws them.
		/// </summary>
		void CreateSplinePipelines();

		/// <summary>
		/// Regenerates the vertices of the splines that changed. it records commands, so it is called while the command list is open
		/// </summary>
		void UpdateSplines();

		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;

	private:
		struct SplineGPUData {
			ref<SplineRenderer> renderer;
			GameEntity* entity = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle = {};
			Ptr<DX12Buffer> vertexBufferData;
			ref<Material> computeMaterial; // holds the curve parameters of the spline
			ref<Compute::DX12ComputePipelineModule> computeModule;
			ref<Rasterization::DX12RasterizationPipelineModule> rasterModule;
		};

		ref<DX12PipelineFactory> m_pipelineFactory;
		ref<DX12GPUResourceManager> m_resourceManager;
		ShaderRegistery* m_shaderProgramRegistery;

		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		Ptr<DX12DepthTexture> m_depthTexture;

		std::vector<SplineGPUData> m_splines;

		std::vector<EntityGBufferData> m_gBufferEntities;
		GBufferResources m_gBuffer;
		ref<Material> m_gBufferMaterial;
		ref<Rasterization::DX12RasterizationPipelineModule> m_gBufferRasterization; // draws the g buffer renderers into the g buffer
		ref<Material> m_gBufferRTMaterial; // the global material of the ray tracing pipeline, that points to it
		Ptr<DX12Texture> m_rtOutputTexture; // written by the ray tracing stage, sampled by the third stage
		ref<RayTracing::DX12RayTracingPipelineModule> m_GBufferrayTracingPipeline;
		float m_hybridBackgroundColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

		Ptr<DX12RenderPass> m_gBufferRenderPass;
		Ptr<DX12RenderPass> m_mainRenderPass;
	};
}
