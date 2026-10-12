#include "pch.h"
#include "DX12ComputePass.h"
#include "Compute/DX12ComputePipelineModule.h"
#include "RayTracing/DX12RayTracingPipelineModule.h"

namespace LuxonEngine::Rendering::DX12 {
	void DX12ComputePass::AddPipeline(const ref<Compute::DX12ComputePipelineModule>& pipeline)
	{
		if (pipeline != nullptr)
			m_computeSteps.push_back(pipeline);
	}

	void DX12ComputePass::AddPipeline(const ref<RayTracing::DX12RayTracingPipelineModule>& pipeline)
	{
		if (pipeline != nullptr)
			m_rayTracingSteps.push_back(pipeline);
	}

	void DX12ComputePass::Execute(ID3D12GraphicsCommandList7* commandList)
	{
		for (auto& step : m_computeSteps) {
			step->Dispatch(commandList);
		}
		for (auto& step : m_rayTracingSteps) {
			step->Dispatch(commandList);
		}
	}
}
