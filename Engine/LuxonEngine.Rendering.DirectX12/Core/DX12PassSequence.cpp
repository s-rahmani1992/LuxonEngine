#include "pch.h"
#include "DX12PassSequence.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12ResourceState.h"
#include <Core/Logger.h>
#include <string>
#include <unordered_map>

namespace LuxonEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	namespace {
		struct ResourceUse
		{
			UInt32 passIndex;
			D3D12_RESOURCE_STATES state;
		};

		// A barrier needed before the pass: a transition, or a UAV barrier when the resource stays in UNORDERED_ACCESS.
		struct StateChange
		{
			UInt32 passIndex;
			D3D12_RESOURCE_STATES before;
			D3D12_RESOURCE_STATES after;
			bool uavBarrier;
		};

		struct TrackedResource
		{
			ID3D12Resource* resource = nullptr;
			DX12Texture* texture = nullptr;
			DX12Buffer* buffer = nullptr;
			D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
			std::vector<ResourceUse> uses;
		};

		// Returns false when the resource needs no barrier to be used in the required state.
		bool NeedsBarrier(D3D12_RESOURCE_STATES current, D3D12_RESOURCE_STATES required, D3D12_RESOURCE_STATES& next, bool& uavBarrier)
		{
			next = required;
			uavBarrier = false;

			if (current == required) {
				// the previous pass may still be writing the resource
				uavBarrier = (required & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != D3D12_RESOURCE_STATE_COMMON;
				return uavBarrier;
			}

			if (IsReadOnlyState(current) && IsReadOnlyState(required)) {
				if ((current & required) == required)
					return false;

				next = current | required;
			}

			return true;
		}

		// Walks the uses of a resource from the start state, returns the state after the last use.
		D3D12_RESOURCE_STATES Simulate(const std::vector<ResourceUse>& uses, D3D12_RESOURCE_STATES start, std::vector<StateChange>* changes)
		{
			D3D12_RESOURCE_STATES current = start;

			for (auto& use : uses) {
				D3D12_RESOURCE_STATES next;
				bool uavBarrier;

				if (NeedsBarrier(current, use.state, next, uavBarrier) == false)
					continue;

				if (changes != nullptr)
					changes->push_back(StateChange{ use.passIndex, current, next, uavBarrier });

				current = next;
			}

			return current;
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

		D3D12_RESOURCE_BARRIER CreateBarrier(ID3D12Resource* resource, const StateChange& change)
		{
			if (change.uavBarrier) {
				return D3D12_RESOURCE_BARRIER{
					.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
					.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.UAV = D3D12_RESOURCE_UAV_BARRIER{ .pResource = resource },
				};
			}

			return CreateTransitionBarrier(resource, change.before, change.after);
		}

		void Submit(ID3D12GraphicsCommandList7* commandList, const std::vector<D3D12_RESOURCE_BARRIER>& barriers)
		{
			if (barriers.empty() == false)
				commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
		}
	}

	void DX12PassSequence::AddPass(DX12Pass* pass)
	{
		if (pass != nullptr)
			m_passes.push_back(pass);

		m_compiled = false;
	}

	void DX12PassSequence::SetSwapChain(IDXGISwapChain3* swapChain)
	{
		m_swapChain = swapChain;
		m_compiled = false;
	}

	void DX12PassSequence::Clear()
	{
		m_passes.clear();
		m_swapChain = nullptr;
		m_passBarriers.clear();
		m_beginBarriers.clear();
		m_endBarriers.clear();
		m_prologueBarriers.clear();
		m_entryStates.clear();
		m_compiled = false;
		m_prologueDone = false;
	}

	bool DX12PassSequence::Compile()
	{
		m_compiled = false;
		m_prologueDone = false;
		m_passBarriers.clear();
		m_beginBarriers.clear();
		m_endBarriers.clear();
		m_prologueBarriers.clear();
		m_entryStates.clear();

		UInt32 backBufferCount = 1;
		std::vector<ID3D12Resource*> backBuffers;

		if (m_swapChain != nullptr) {
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

			if (FAILED(m_swapChain->GetDesc1(&swapChainDesc))) {
				Logger::LogError("The pass sequence cannot read the description of the swap chain");
				return false;
			}

			backBufferCount = swapChainDesc.BufferCount;

			for (UInt32 i = 0; i < backBufferCount; i++) {
				ComPtr<ID3D12Resource> backBuffer;

				if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)))) {
					Logger::LogError("The pass sequence cannot get a back buffer of the swap chain");
					return false;
				}

				backBuffers.push_back(backBuffer.Get());
			}
		}

		const UInt32 passCount = static_cast<UInt32>(m_passes.size());
		m_passBarriers.assign(backBufferCount, std::vector<std::vector<D3D12_RESOURCE_BARRIER>>(passCount));
		m_beginBarriers.assign(backBufferCount, {});
		m_endBarriers.assign(backBufferCount, {});

		// the uses of every resource in the order of the passes
		std::vector<TrackedResource> trackedResources;
		std::unordered_map<ID3D12Resource*, size_t> trackedIndices;
		std::vector<ResourceUse> backBufferUses;

		for (UInt32 passIndex = 0; passIndex < passCount; passIndex++) {
			for (auto& usage : m_passes[passIndex]->GetUsages()) {
				auto [it, added] = trackedIndices.try_emplace(usage.resource, trackedResources.size());

				if (added) {
					// the recorded state of the resource is its state before the first frame
					D3D12_RESOURCE_STATES initialState = usage.texture != nullptr ? usage.texture->GetState() : usage.buffer->GetState();

					trackedResources.push_back(TrackedResource{
						.resource = usage.resource,
						.texture = usage.texture,
						.buffer = usage.buffer,
						.initialState = initialState,
						});
				}

				trackedResources[it->second].uses.push_back(ResourceUse{ passIndex, usage.state });
			}

			if (m_passes[passIndex]->UsesBackBuffer()) {
				if (m_swapChain == nullptr) {
					Logger::LogError("A pass uses the back buffer but the pass sequence has no swap chain");
					return false;
				}

				backBufferUses.push_back(ResourceUse{ passIndex, m_passes[passIndex]->GetBackBufferState() });
			}
		}

		for (auto& tracked : trackedResources) {
			D3D12_RESOURCE_STATES entryState = tracked.initialState;
			bool settled = false;

			for (int iteration = 0; iteration < 4 && settled == false; iteration++) {
				D3D12_RESOURCE_STATES endState = Simulate(tracked.uses, entryState, nullptr);
				settled = endState == entryState;
				entryState = endState;
			}

			if (settled == false)
				Logger::LogWarning("The states of a resource do not settle between frames, its barriers may be wrong");

			std::vector<StateChange> changes;
			Simulate(tracked.uses, entryState, &changes);

			// the first frame starts in the recorded state, the other frames in the entry state
			if (tracked.initialState != entryState)
				m_prologueBarriers.push_back(CreateTransitionBarrier(tracked.resource, tracked.initialState, entryState));

			if (tracked.texture != nullptr || tracked.buffer != nullptr)
				m_entryStates.push_back(EntryState{ .texture = tracked.texture, .buffer = tracked.buffer, .state = entryState });

			for (auto& change : changes) {
				D3D12_RESOURCE_BARRIER barrier = CreateBarrier(tracked.resource, change);

				for (auto& barriersOfBackBuffer : m_passBarriers)
					barriersOfBackBuffer[change.passIndex].push_back(barrier);
			}
		}

		if (backBufferUses.empty() == false) {
			std::vector<StateChange> changes;
			D3D12_RESOURCE_STATES endState = Simulate(backBufferUses, D3D12_RESOURCE_STATE_PRESENT, &changes);

			for (UInt32 backBufferIndex = 0; backBufferIndex < backBufferCount; backBufferIndex++) {
				for (auto& change : changes) {
					D3D12_RESOURCE_BARRIER barrier = CreateBarrier(backBuffers[backBufferIndex], change);

					if (change.passIndex == backBufferUses.front().passIndex)
						m_beginBarriers[backBufferIndex].push_back(barrier);
					else
						m_passBarriers[backBufferIndex][change.passIndex].push_back(barrier);
				}

				if (endState != D3D12_RESOURCE_STATE_PRESENT)
					m_endBarriers[backBufferIndex].push_back(CreateTransitionBarrier(backBuffers[backBufferIndex], endState, D3D12_RESOURCE_STATE_PRESENT));
			}
		}

		if (m_logPlan) {
			std::string plan = "Pass sequence: " + std::to_string(passCount) + " passes, " + std::to_string(m_prologueBarriers.size()) + " prologue barriers, barriers before each pass:";

			for (UInt32 passIndex = 0; passIndex < passCount; passIndex++)
				plan += " " + std::to_string(m_passBarriers[0][passIndex].size());

			Logger::Log(plan);
		}

		m_compiled = true;
		return true;
	}

	void DX12PassSequence::Execute(ID3D12GraphicsCommandList7* commandList)
	{
		if (m_compiled == false) {
			Logger::LogError("The pass sequence is not compiled, the passes are not executed");
			return;
		}

		UInt32 backBufferIndex = m_swapChain != nullptr ? m_swapChain->GetCurrentBackBufferIndex() : 0;

		if (backBufferIndex >= m_passBarriers.size())
			backBufferIndex = 0;

		if (m_prologueDone == false) {
			Submit(commandList, m_prologueBarriers);

			for (auto& entry : m_entryStates) {
				if (entry.texture != nullptr)
					entry.texture->SetState(entry.state);
				else
					entry.buffer->SetState(entry.state);
			}

			m_prologueDone = true;
		}

		Submit(commandList, m_beginBarriers[backBufferIndex]);

		for (UInt32 passIndex = 0; passIndex < m_passes.size(); passIndex++) {
			Submit(commandList, m_passBarriers[backBufferIndex][passIndex]);
			m_passes[passIndex]->Execute(commandList);
		}

		Submit(commandList, m_endBarriers[backBufferIndex]);
	}
}
