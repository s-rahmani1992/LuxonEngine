#include "pch.h"
#include "DX12RenderPass.h"
#include "DX12RenderTexture.h"
#include "DX12DepthTexture.h"
#include "Rasterization/DX12RasterizationPipelineModule.h"
#include "Mesh/DX12MeshPipelineModule.h"

namespace LuxonEngine::Rendering::DX12 {
	namespace {
		D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToBeginningAccess(DX12LoadOp loadOp)
		{
			switch (loadOp) {
			case DX12LoadOp::Load: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
			case DX12LoadOp::Clear: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			default: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
			}
		}

		D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ToEndingAccess(DX12StoreOp storeOp)
		{
			return storeOp == DX12StoreOp::Store ? D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE : D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		}

		D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			return D3D12_RESOURCE_BARRIER{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
					.pResource = resource,
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = before,
					.StateAfter = after,
				},
			};
		}

		bool HasStencil(DXGI_FORMAT depthFormat)
		{
			return depthFormat == DXGI_FORMAT_D24_UNORM_S8_UINT || depthFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		}

		D3D12_RENDER_PASS_RENDER_TARGET_DESC BuildRenderTargetDesc(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, DXGI_FORMAT format, const DX12RenderPassParams& params, size_t index)
		{
			D3D12_RENDER_PASS_RENDER_TARGET_DESC desc = {};
			desc.cpuDescriptor = rtvHandle;
			desc.BeginningAccess.Type = ToBeginningAccess(params.colorLoadOp);

			if (params.colorLoadOp == DX12LoadOp::Clear) {
				desc.BeginningAccess.Clear.ClearValue.Format = format;
				const std::array<float, 4> color = index < params.clearColors.size() ? params.clearColors[index] : std::array<float, 4>{};
				std::copy(color.begin(), color.end(), desc.BeginningAccess.Clear.ClearValue.Color);
			}

			desc.EndingAccess.Type = ToEndingAccess(params.colorStoreOp);
			return desc;
		}
	}

	void DX12RenderPass::Reset()
	{
		m_renderTargetDescs.clear();
		m_depthDesc = {};
		m_hasDepth = false;
		m_swapChain = nullptr;
		m_device = nullptr;
		m_rtvHeap.Reset();
		m_beginBarriers.clear();
		m_endBarriers.clear();
		m_renderTargetTextures.clear();
		m_renderTargetFinalState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_renderTargetCount = 0;
		m_currentBackBuffer = 0;
		m_viewport = {};
		m_scissor = {};
		m_inRenderPass = false;
	}

	bool DX12RenderPass::Initialize(IDXGISwapChain3* swapChain, const DX12RenderPassParams& params)
	{
		Reset();
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		if (swapChain == nullptr || FAILED(swapChain->GetDesc1(&swapChainDesc)))
			return false;

		// the device that owns the back buffers
		ComPtr<ID3D12Resource> firstBuffer;

		if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&firstBuffer))) || FAILED(firstBuffer->GetDevice(IID_PPV_ARGS(&m_device))))
			return false;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = swapChainDesc.BufferCount,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0,
		};
		if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
			Reset();
			return false;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE heapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		const UInt32 incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{
			.Format = swapChainDesc.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = { .MipSlice = 0, .PlaneSlice = 0 },
		};

		for (UInt32 i = 0; i < swapChainDesc.BufferCount; i++) {
			ComPtr<ID3D12Resource> backBuffer;
			if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)))) {
				Reset();
				return false;
			}

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = heapStart;
			rtvHandle.ptr += static_cast<SIZE_T>(i) * incrementSize;
			m_device->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, rtvHandle);

			// every back buffer is cleared with the first clear color
			m_renderTargetDescs.push_back(BuildRenderTargetDesc(rtvHandle, swapChainDesc.Format, params, 0));

			// the barriers of this back buffer, the swap chain keeps the buffer alive so the pointer stays valid
			auto& beginBarriers = m_beginBarriers.emplace_back();
			if (params.backBufferInitialState != D3D12_RESOURCE_STATE_RENDER_TARGET)
				beginBarriers.push_back(CreateTransitionBarrier(backBuffer.Get(), params.backBufferInitialState, D3D12_RESOURCE_STATE_RENDER_TARGET));

			auto& endBarriers = m_endBarriers.emplace_back();
			if (params.backBufferFinalState != D3D12_RESOURCE_STATE_RENDER_TARGET)
				endBarriers.push_back(CreateTransitionBarrier(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, params.backBufferFinalState));
		}

		m_swapChain = swapChain;
		m_renderTargetCount = 1;
		SetDefaultViewport(swapChainDesc.Width, swapChainDesc.Height);
		BuildDepthDesc(params);
		return true;
	}

	bool DX12RenderPass::Initialize(const std::vector<DX12RenderTexture*>& renderTargets, const DX12RenderPassParams& params)
	{
		Reset();
		for (DX12RenderTexture* target : renderTargets) {
			if (target == nullptr || m_renderTargetDescs.size() >= MaxRenderTargets)
				continue;

			if (m_renderTargetDescs.empty())
				SetDefaultViewport(target->GetWidth(), target->GetHeight());

			m_renderTargetDescs.push_back(BuildRenderTargetDesc(target->GetRtvHandle(), target->GetFormat(), params, m_renderTargetDescs.size()));
			AddTransition(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_renderTargetTextures.push_back(target);
		}

		// the barriers that leave the targets in their final state, filled once
		if (params.renderTargetFinalState != D3D12_RESOURCE_STATE_RENDER_TARGET && !m_renderTargetTextures.empty()) {
			m_renderTargetFinalState = params.renderTargetFinalState;
			auto& endBarriers = m_endBarriers.emplace_back();
			for (DX12RenderTexture* target : m_renderTargetTextures)
				endBarriers.push_back(CreateTransitionBarrier(target->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, m_renderTargetFinalState));
		}

		m_renderTargetCount = static_cast<UInt32>(m_renderTargetDescs.size());
		BuildDepthDesc(params);

		// a pass with only a depth target is valid (e.g. a shadow map)
		return !m_renderTargetDescs.empty() || m_hasDepth;
	}

	void DX12RenderPass::BuildDepthDesc(const DX12RenderPassParams& params)
	{
		DX12DepthTexture* depthTarget = params.depthTarget;
		if (depthTarget == nullptr)
			return;

		m_hasDepth = true;
		AddTransition(depthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		if (m_renderTargetDescs.empty())
			SetDefaultViewport(depthTarget->GetWidth(), depthTarget->GetHeight());

		const DXGI_FORMAT depthFormat = depthTarget->GetFormat();
		m_depthDesc = {};
		m_depthDesc.cpuDescriptor = depthTarget->GetDsvHandle();

		m_depthDesc.DepthBeginningAccess.Type = ToBeginningAccess(params.depthLoadOp);
		if (params.depthLoadOp == DX12LoadOp::Clear) {
			m_depthDesc.DepthBeginningAccess.Clear.ClearValue.Format = depthFormat;
			m_depthDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = params.clearDepth;
		}
		m_depthDesc.DepthEndingAccess.Type = ToEndingAccess(params.depthStoreOp);

		if (HasStencil(depthFormat)) {
			m_depthDesc.StencilBeginningAccess.Type = ToBeginningAccess(params.depthLoadOp);
			if (params.depthLoadOp == DX12LoadOp::Clear) {
				m_depthDesc.StencilBeginningAccess.Clear.ClearValue.Format = depthFormat;
				m_depthDesc.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = params.clearStencil;
			}
			m_depthDesc.StencilEndingAccess.Type = ToEndingAccess(params.depthStoreOp);
		}
		else {
			m_depthDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
			m_depthDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
		}
	}

	void DX12RenderPass::SetDefaultViewport(UInt32 width, UInt32 height)
	{
		m_viewport = D3D12_VIEWPORT{
			.TopLeftX = 0.0f, .TopLeftY = 0.0f,
			.Width = static_cast<float>(width), .Height = static_cast<float>(height),
			.MinDepth = 0.0f, .MaxDepth = 1.0f,
		};
		m_scissor = D3D12_RECT{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	}

	void DX12RenderPass::AddPipeline(const ref<Rasterization::DX12RasterizationPipelineModule>& pipeline)
	{
		if (pipeline != nullptr)
			m_pipelines.push_back(pipeline);
	}

	void DX12RenderPass::AddPipeline(const ref<MeshShading::DX12MeshPipelineModule>& pipeline)
	{
		if (pipeline != nullptr)
			m_meshPipelines.push_back(pipeline);
	}

	void DX12RenderPass::SetViewport(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissor)
	{
		m_viewport = viewport;
		m_scissor = scissor;
	}

	void DX12RenderPass::BeginRenderPass(ID3D12GraphicsCommandList7* commandList)
	{
		const D3D12_RENDER_PASS_RENDER_TARGET_DESC* renderTargetDescs = m_renderTargetDescs.data();

		if (m_swapChain != nullptr) {
			m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();

			auto& barriers = m_beginBarriers[m_currentBackBuffer];
			if (!barriers.empty())
				commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

			renderTargetDescs = &m_renderTargetDescs[m_currentBackBuffer];
		}

		commandList->BeginRenderPass(m_renderTargetCount, renderTargetDescs,
			m_hasDepth ? &m_depthDesc : nullptr, D3D12_RENDER_PASS_FLAG_NONE);
		m_inRenderPass = true;

		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissor);
	}

	void DX12RenderPass::EndRenderPass(ID3D12GraphicsCommandList7* commandList)
	{
		if (!m_inRenderPass)
			return;

		commandList->EndRenderPass();
		m_inRenderPass = false;

		if (m_swapChain != nullptr) {
			auto& barriers = m_endBarriers[m_currentBackBuffer];
			if (!barriers.empty())
				commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
		}
		else if (!m_endBarriers.empty()) {
			commandList->ResourceBarrier(static_cast<UINT>(m_endBarriers[0].size()), m_endBarriers[0].data());

			for (DX12RenderTexture* target : m_renderTargetTextures)
				target->SetState(m_renderTargetFinalState);
		}
	}

	void DX12RenderPass::Record(ID3D12GraphicsCommandList7* commandList)
	{
		BeginRenderPass(commandList);

		// every pipeline binds its own descriptor heap
		for (auto& pipeline : m_pipelines)
			pipeline->Draw(commandList);

		for (auto& pipeline : m_meshPipelines)
			pipeline->Dispatch(commandList);

		EndRenderPass(commandList);
	}
}
