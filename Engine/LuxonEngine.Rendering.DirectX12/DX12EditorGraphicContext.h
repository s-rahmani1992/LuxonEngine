#pragma once
#include "DX12GraphicContext.h"
#include "DX12HybridContext.h"

namespace LuxonEngine::Rendering::DX12 {

	class DX12GameEntityPipelineModule;

	namespace Rasterization {
		class DX12RasterizationMaterial;
	}

	class DX12EditorGraphicContext : public DX12GraphicContext
	{
	public:
		DX12EditorGraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<LuxonEngine::Platform::GraphicWindow>& window, const ref<DX12AssetManager>& assetManager)
			: DX12GraphicContext(bufferCount, commandExecuter, window, assetManager) {
		}

		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		bool InitializeDepthBuffer();
		void InitializePipelines(const ref<Material>& overrideMaterial);
		void SyncEntities();

	private:
		// Depth Stencil
		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_depthStencilvHeap;

		ComPtr<ID3D12DescriptorHeap> m_rasterHeap;

		std::vector<DX12MeshRendererGPUData> m_meshRendererData;
		std::vector<ref<DX12GameEntityPipelineModule>> m_rasterizationPipelines;

		ref<Material> m_overrideMaterial;
		ref<Scene> m_scene;
	};
}