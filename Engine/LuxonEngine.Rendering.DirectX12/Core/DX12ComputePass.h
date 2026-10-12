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

		virtual void Execute(ID3D12GraphicsCommandList7* commandList) override;

	private:
		std::vector<ref<Compute::DX12ComputePipelineModule>> m_computeSteps;
		std::vector<ref<RayTracing::DX12RayTracingPipelineModule>> m_rayTracingSteps;
	};
}
