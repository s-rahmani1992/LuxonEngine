#pragma once
#include "DX12GraphicContext.h"

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
		class DX12RasterizationMaterial;
		class DX12RasterizationPipelineModule;
	}

	class DX12SplineRasterPipelineModule;
	class DX12PipelineFactory;

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

		DXGI_FORMAT formats[TargetCount] = { DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R8_UINT };
		ComPtr<ID3D12Resource2> buffers[TargetCount];
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[TargetCount] = {};
		ComPtr<ID3D12DescriptorHeap> srvHeaps[TargetCount]; // shader visible, read by the ray tracing stage
		ComPtr<ID3D12DescriptorHeap> cpuSrvHeap; // non shader visible views, the source of the descriptor copies
		D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandles[TargetCount] = {};
		ComPtr<ID3D12Resource2> depthBuffer;
		ComPtr<ID3D12DescriptorHeap> depthHeap;
	};

	struct SplineRendererData {
	public:
		ref<SplineRenderer> renderer;
		ref<Rasterization::DX12RasterizationMaterial> material;
		D3D12_GPU_DESCRIPTOR_HANDLE transformHandle;
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
		void InitializePipelines();
		bool InitializeGBuffer();
		void RenderGBuffer();

		std::vector<ref<MeshShading::DX12MeshPipelineModule>> CreateSurfaceInstancePipelines();

		/// <summary>
		/// Creates one rasterization pipeline per material of the mesh renderers and g buffer renderers of the entities
		/// </summary>
		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> CreateRasterizationPipelines();
		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;

	private:
		ref<DX12PipelineFactory> m_pipelineFactory;
		ShaderRegistery* m_shaderProgramRegistery;

		// Depth Stencil
		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_depthStencilvHeap;

		ComPtr<ID3D12DescriptorHeap> m_rasterHeap;

		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> m_rasterizationModules;
		std::vector<ref<DX12SplineRasterPipelineModule>> m_splinePipelines;
		std::vector<ref<MeshShading::DX12MeshPipelineModule>> m_meshShadingPipelines;

		std::vector<EntityGBufferData> m_gBufferEntities;
		GBufferResources m_gBuffer;
		ref<Material> m_gBufferMaterial;
		ref<Rasterization::DX12RasterizationPipelineModule> m_gBufferRasterization; // draws the g buffer renderers into the g buffer
		ComPtr<ID3D12DescriptorHeap> m_rtOutputCpuHeap; // CPU only view of the ray tracing output, the source of the descriptor copies
		ref<RayTracing::DX12RayTracingPipelineModule> m_GBufferrayTracingPipeline;
		float m_hybridBackgroundColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	};
}

