#pragma once
#include "DX12Pass.h"
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	namespace Compute {
		class DX12ComputePipelineModule;
	}

	namespace RayTracing {
		class DX12RayTracingPipelineModule;
	}

	class DX12ComputePass : public DX12Pass
	{
	public:
		void AddPipeline(const ref<Compute::DX12ComputePipelineModule>& pipeline);

		void AddPipeline(const ref<RayTracing::DX12RayTracingPipelineModule>& pipeline);

		void AddResource(DX12Texture* texture, D3D12_RESOURCE_STATES usedState);
		void AddResource(DX12Texture* texture, D3D12_RESOURCE_STATES usedState, D3D12_RESOURCE_STATES finalState);
		void AddResource(DX12Buffer* buffer, D3D12_RESOURCE_STATES usedState);
		void AddResource(DX12Buffer* buffer, D3D12_RESOURCE_STATES usedState, D3D12_RESOURCE_STATES finalState);

		// Removes the pipelines and the resources, for a scene that is set up again.
		void Reset();

		inline UInt32 GetPipelineCount() const { return static_cast<UInt32>(m_steps.size()); }

	protected:
		virtual void Record(ID3D12GraphicsCommandList7* commandList) override;

	private:
		struct Step
		{
			ref<Compute::DX12ComputePipelineModule> computePipeline;
			ref<RayTracing::DX12RayTracingPipelineModule> rayTracingPipeline;
		};

		struct FinalState
		{
			DX12Texture* texture = nullptr;
			DX12Buffer* buffer = nullptr;
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		};

		std::vector<Step> m_steps;

		std::vector<D3D12_RESOURCE_BARRIER> m_endBarriers;
		std::vector<FinalState> m_finalStates;
	};
}
