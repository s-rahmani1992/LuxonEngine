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

	// What happens to the content of an attachment when the pass begins.
	enum class DX12LoadOp
	{
		Load,     // keep the existing content
		Clear,    // clear to the clear value
		DontCare, // the content is undefined, the pass overwrites everything
	};

	// What happens to the content of an attachment when the pass ends.
	enum class DX12StoreOp
	{
		Store,    // keep the result, needed when it is read later
		DontCare, // the result is not needed afterwards
	};

	struct DX12RenderPassParams
	{
		DX12DepthTexture* depthTarget = nullptr;

		DX12LoadOp colorLoadOp = DX12LoadOp::Clear;
		DX12StoreOp colorStoreOp = DX12StoreOp::Store;
		std::vector<std::array<float, 4>> clearColors;

		D3D12_RESOURCE_STATES backBufferInitialState = D3D12_RESOURCE_STATE_PRESENT;
		D3D12_RESOURCE_STATES backBufferFinalState = D3D12_RESOURCE_STATE_PRESENT;

		D3D12_RESOURCE_STATES renderTargetFinalState = D3D12_RESOURCE_STATE_RENDER_TARGET;

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

		void SetViewport(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor);

		void BeginRenderPass(ID3D12GraphicsCommandList7* commandList);
		void EndRenderPass(ID3D12GraphicsCommandList7* commandList);

		inline bool IsSwapChainPass() const { return m_swapChain != nullptr; }
		inline UInt32 GetPipelineCount() const { return static_cast<UInt32>(m_pipelines.size()); }

	protected:
		virtual void Record(ID3D12GraphicsCommandList7* commandList) override;

	private:
		void Reset();
		void BuildDepthDesc(const DX12RenderPassParams& params);
		void SetDefaultViewport(UInt32 width, UInt32 height);

		// One description per render target, or one per back buffer for a swap chain pass.
		std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> m_renderTargetDescs;
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_depthDesc = {};
		bool m_hasDepth = false;

		IDXGISwapChain3* m_swapChain = nullptr;   // not owned, the caller keeps it alive
		ID3D12Device10* m_device = nullptr;       // not owned
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;   // render target views of the back buffers
		UInt32 m_currentBackBuffer = 0;
		UInt32 m_renderTargetCount = 0;

		D3D12_VIEWPORT m_viewport = {};
		D3D12_RECT m_scissor = {};

		std::vector<std::vector<D3D12_RESOURCE_BARRIER>> m_beginBarriers;
		std::vector<std::vector<D3D12_RESOURCE_BARRIER>> m_endBarriers;

		// Render texture passes only: the targets whose state is updated when the end barriers are submitted.
		std::vector<DX12RenderTexture*> m_renderTargetTextures;
		D3D12_RESOURCE_STATES m_renderTargetFinalState = D3D12_RESOURCE_STATE_RENDER_TARGET;

		bool m_inRenderPass = false;

		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> m_pipelines;
		std::vector<ref<MeshShading::DX12MeshPipelineModule>> m_meshPipelines;
	};
}
