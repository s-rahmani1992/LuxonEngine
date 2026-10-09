#include "pch.h"
#include "DX12Pass.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"

namespace LuxonEngine::Rendering::DX12 {
	void DX12Pass::AddTransition(DX12Texture* texture, D3D12_RESOURCE_STATES requiredState)
	{
		if (texture == nullptr)
			return;
		m_transitions.push_back(Transition{ .texture = texture, .requiredState = requiredState });
	}

	void DX12Pass::AddTransition(DX12Buffer* buffer, D3D12_RESOURCE_STATES requiredState)
	{
		if (buffer == nullptr)
			return;
		m_transitions.push_back(Transition{ .buffer = buffer, .requiredState = requiredState });
	}

	void DX12Pass::Execute(ID3D12GraphicsCommandList7* commandList)
	{
		ApplyTransitions(commandList);
		Record(commandList);
	}

	void DX12Pass::ApplyTransitions(ID3D12GraphicsCommandList7* commandList)
	{
		std::vector<D3D12_RESOURCE_BARRIER> barriers;
		barriers.reserve(m_transitions.size());

		for (auto& transition : m_transitions) {
			ID3D12Resource* resource = transition.texture ? static_cast<ID3D12Resource*>(transition.texture->GetResource())
				: static_cast<ID3D12Resource*>(transition.buffer->GetResource());
			if (resource == nullptr)
				continue;

			const D3D12_RESOURCE_STATES currentState = transition.texture ? transition.texture->GetState() : transition.buffer->GetState();

			if (currentState == transition.requiredState) {
				// The previous pass may still be writing the resource, so the reads and writes of this pass must wait for it
				if (currentState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
					barriers.push_back(D3D12_RESOURCE_BARRIER{
						.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
						.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
						.UAV = D3D12_RESOURCE_UAV_BARRIER{ .pResource = resource },
						});
				}
				continue;
			}

			barriers.push_back(D3D12_RESOURCE_BARRIER{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
					.pResource = resource,
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = currentState,
					.StateAfter = transition.requiredState,
				},
				});

			if (transition.texture)
				transition.texture->SetState(transition.requiredState);
			else
				transition.buffer->SetState(transition.requiredState);
		}

		if (!barriers.empty())
			commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}
}
