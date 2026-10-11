#include "pch.h"
#include "DX12ComputePass.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "Compute/DX12ComputePipelineModule.h"
#include "RayTracing/DX12RayTracingPipelineModule.h"

namespace LuxonEngine::Rendering::DX12 {
	void DX12ComputePass::AddPipeline(const ref<Compute::DX12ComputePipelineModule>& pipeline)
	{
		if (pipeline != nullptr)
			m_steps.push_back(Step{ .computePipeline = pipeline });
	}

	void DX12ComputePass::AddPipeline(const ref<RayTracing::DX12RayTracingPipelineModule>& pipeline)
	{
		if (pipeline != nullptr)
			m_steps.push_back(Step{ .rayTracingPipeline = pipeline });
	}

	void DX12ComputePass::AddResource(DX12Texture* texture, D3D12_RESOURCE_STATES usedState)
	{
		AddResource(texture, usedState, usedState);
	}

	void DX12ComputePass::AddResource(DX12Texture* texture, D3D12_RESOURCE_STATES usedState, D3D12_RESOURCE_STATES finalState)
	{
		if (texture == nullptr || texture->GetResource() == nullptr)
			return;

		AddTransition(texture, usedState);

		if (finalState != usedState) {
			m_endBarriers.push_back(CreateTransitionBarrier(texture->GetResource(), usedState, finalState));
			m_finalStates.push_back(FinalState{ .texture = texture, .state = finalState });
		}
	}

	void DX12ComputePass::AddResource(DX12Buffer* buffer, D3D12_RESOURCE_STATES usedState)
	{
		AddResource(buffer, usedState, usedState);
	}

	void DX12ComputePass::AddResource(DX12Buffer* buffer, D3D12_RESOURCE_STATES usedState, D3D12_RESOURCE_STATES finalState)
	{
		if (buffer == nullptr || buffer->GetResource() == nullptr)
			return;

		AddTransition(buffer, usedState);

		if (finalState != usedState) {
			m_endBarriers.push_back(CreateTransitionBarrier(buffer->GetResource(), usedState, finalState));
			m_finalStates.push_back(FinalState{ .buffer = buffer, .state = finalState });
		}
	}

	void DX12ComputePass::Reset()
	{
		m_steps.clear();
		m_endBarriers.clear();
		m_finalStates.clear();
		ClearTransitions();
	}

	void DX12ComputePass::Record(ID3D12GraphicsCommandList7* commandList)
	{
		// every pipeline binds its own descriptor heap
		for (auto& step : m_steps) {
			if (step.computePipeline != nullptr) {
				step.computePipeline->Dispatch(commandList);
			}
			else {
				step.rayTracingPipeline->Dispatch(commandList);
			}
		}

		if (m_endBarriers.empty())
			return;

		commandList->ResourceBarrier(static_cast<UINT>(m_endBarriers.size()), m_endBarriers.data());

		for (auto& finalState : m_finalStates) {
			if (finalState.texture)
				finalState.texture->SetState(finalState.state);
			else
				finalState.buffer->SetState(finalState.state);
		}
	}
}
