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
	}

	class DX12GBufferPipelineModule;
	class DX12GameEntityPipelineModule;
	class DX12SplineRasterPipelineModule;
	class DX12PipelineFactory;

	struct DX12MeshRendererGPUData {
	public:
		ref<MeshRenderer> meshRenderer;
		ref<Rasterization::DX12RasterizationMaterial> material;
		ComPtr<ID3D12Resource2> transformResource;
		D3D12_GPU_DESCRIPTOR_HANDLE transformHandle;
	};

	struct EntityGBufferData {
	public:
		ref<GBufferRTReflectionRenderer> renderer;
		ComPtr<ID3D12Resource2> transformResource;
		D3D12_GPU_DESCRIPTOR_HANDLE transformHandle;
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
		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;

	private:
		ref<DX12PipelineFactory> m_pipelineFactory;
		ShaderRegistery* m_shaderProgramRegistery;

		// Depth Stencil
		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_depthStencilvHeap;

		ComPtr<ID3D12DescriptorHeap> m_rasterHeap;

		std::vector<DX12MeshRendererGPUData> m_meshRendererData;
		std::vector<ref<DX12GameEntityPipelineModule>> m_rasterizationPipelines;
		std::vector<ref<DX12SplineRasterPipelineModule>> m_splinePipelines;

		std::vector<EntityGBufferData> m_gBufferEntities;
		ref<DX12GBufferPipelineModule> m_gBufferPipeline;
		ref<RayTracing::DX12RayTracingPipelineModule> m_GBufferrayTracingPipeline;
		float m_hybridBackgroundColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	};
}

