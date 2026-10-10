#pragma once
#include "DX12GraphicContext.h"
#include "Core/DX12Texture.h"

namespace LuxonEngine::Rendering::DX12
{
	class DX12GPUResourceManager;

	namespace RayTracing {
		class DX12RayTracingPipelineModule;
	}

	class DX12RayTracingContext : public DX12GraphicContext
	{
	public:
		DX12RayTracingContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<LuxonEngine::Platform::GraphicWindow>& window, const ref<DX12AssetManager>& assetManager,
			const ref<DX12PipelineFactory>& pipelineFactory)
			: DX12GraphicContext(bufferCount, m_commandExecuter, window, assetManager), m_pipelineFactory(pipelineFactory) {}
		
		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		void PrepareRayTracingPipeline(const ref<Material>& rtGlobalMaterial);
		void TransitionOutput(D3D12_RESOURCE_STATES state);

	private:
		ref<DX12PipelineFactory> m_pipelineFactory;
		ref<DX12GPUResourceManager> m_resourceManager;
		ref<Material> m_rtGlobalMaterial; // the pipeline points to it
		Ptr<DX12Texture> m_rtOutputTexture; // the global program writes the image into it, then it is copied to the back buffer
		ref<RayTracing::DX12RayTracingPipelineModule> m_rayTracingPipeline;
	};
}


