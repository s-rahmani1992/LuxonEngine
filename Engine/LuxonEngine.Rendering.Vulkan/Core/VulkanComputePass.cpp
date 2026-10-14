#include "vulkan-pch.h"
#include "VulkanComputePass.h"
#include "Compute/VulkanComputePipelineModule.h"
#include "RayTracing/VulkanRayTracingPipelineModule.h"

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanComputePass::AddPipeline(Compute::VulkanComputePipelineModule* pipeline)
	{
		if (pipeline != nullptr)
			m_computeSteps.push_back(pipeline);
	}

	void VulkanComputePass::AddPipeline(RayTracing::VulkanRayTracingPipelineModule* pipeline)
	{
		if (pipeline != nullptr)
			m_rayTracingSteps.push_back(pipeline);
	}

	void VulkanComputePass::Execute(VkCommandBuffer commandBuffer)
	{
		for (auto* step : m_computeSteps)
			step->Dispatch(commandBuffer);

		for (auto* step : m_rayTracingSteps) {
			step->UpdateTLAS(commandBuffer);
			step->RenderCommand(commandBuffer);
		}
	}
}
