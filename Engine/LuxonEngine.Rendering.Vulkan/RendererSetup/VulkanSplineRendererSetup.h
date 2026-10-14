#pragma once
#include "Core/VulkanRendererSetup.h"
#include "Core/VulkanBuffer.h"
#include <vector>

namespace LuxonEngine::Rendering {
	class SplineRenderer;
}

namespace LuxonEngine::Rendering::Vulkan {
	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	namespace Compute {
		class VulkanComputePipelineModule;
	}

	class VulkanSplineRendererSetup : public IVulkanRendererSetup
	{
	public:
		virtual void Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context) override;
		virtual void Update() override;
		virtual void Teardown() override;

	private:
		struct SplineGPUData {
			SplineRenderer* renderer = nullptr;
			GameEntity* entity = nullptr;
			VkBuffer transformBuffer = VK_NULL_HANDLE;
			UInt32 vertexCount = 0;
			Ptr<VulkanBuffer> vertexBuffer;
			Rasterization::VulkanRasterizationPipelineModule* rasterModule = nullptr;
		};

		std::vector<SplineGPUData> m_splines;
		ref<Compute::VulkanComputePipelineModule> m_computeModule;
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_pipelines;
	};
}
