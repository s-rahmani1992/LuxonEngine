#pragma once
#include "DX12Pass.h"
#include <array>
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	class DX12RenderTexture;
	class DX12DepthTexture;

	namespace Rasterization {
		class DX12RasterizationPipelineModule;
	}

	namespace MeshShading {
		class DX12MeshPipelineModule;
	}

	enum class DX12LoadOp
	{
		Load,
		Clear,
		DontCare,
	};

	enum class DX12StoreOp
	{
		Store,
		DontCare,
	};

	struct DX12RenderPassParams
	{
		DX12LoadOp colorLoadOp = DX12LoadOp::Clear;
		DX12StoreOp colorStoreOp = DX12StoreOp::Store;
		std::vector<std::array<float, 4>> clearColors;

		DX12DepthTexture* depthTarget = nullptr;
		DX12LoadOp depthLoadOp = DX12LoadOp::Clear;
		DX12StoreOp depthStoreOp = DX12StoreOp::Store;
		float clearDepth = 1.0f;
		UInt8 clearStencil = 0;
	};

	class DX12RenderPass : public DX12Pass
	{
	public:
		static constexpr UInt32 MaxRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

		bool Initialize(IDXGISwapChain3* swapChain, const DX12RenderPassParams& params = {});
		bool Initialize(const std::vector<DX12RenderTexture*>& renderTargets, const DX12RenderPassParams& params = {});

		void AddPipeline(const ref<Rasterization::DX12RasterizationPipelineModule>& pipeline);
		void AddPipeline(const ref<MeshShading::DX12MeshPipelineModule>& pipeline);

		virtual void Execute(ID3D12GraphicsCommandList7* commandList) override;

	private:
		void Reset();

		void BeginRenderPass(ID3D12GraphicsCommandList7* commandList);
		void EndRenderPass(ID3D12GraphicsCommandList7* commandList);

		void BuildDepthDesc(const DX12RenderPassParams& params);
		void SetDefaultViewport(UInt32 width, UInt32 height);

		std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> m_renderTargetDescs;
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_depthDesc = {};
		bool m_hasDepth = false;

		IDXGISwapChain3* m_swapChain = nullptr;
		ID3D12Device10* m_device = nullptr;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		UInt32 m_currentBackBuffer = 0;
		UInt32 m_renderTargetCount = 0;

		D3D12_VIEWPORT m_viewport = {};
		D3D12_RECT m_scissor = {};

		bool m_inRenderPass = false;

		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> m_pipelines;
		std::vector<ref<MeshShading::DX12MeshPipelineModule>> m_meshPipelines;
	};
}
