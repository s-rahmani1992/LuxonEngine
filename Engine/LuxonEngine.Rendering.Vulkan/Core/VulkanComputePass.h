#pragma once
#include "VulkanPass.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	namespace Compute {
		class VulkanComputePipelineModule;
	}

	namespace RayTracing {
		class VulkanRayTracingPipelineModule;
	}

	class VulkanComputePass : public VulkanPass
	{
	public:
		void AddPipeline(Compute::VulkanComputePipelineModule* pipeline);
		void AddPipeline(RayTracing::VulkanRayTracingPipelineModule* pipeline);

		virtual void Execute(VkCommandBuffer commandBuffer) override;

	private:
		std::vector<Compute::VulkanComputePipelineModule*> m_computeSteps;
		std::vector<RayTracing::VulkanRayTracingPipelineModule*> m_rayTracingSteps;
	};
}
