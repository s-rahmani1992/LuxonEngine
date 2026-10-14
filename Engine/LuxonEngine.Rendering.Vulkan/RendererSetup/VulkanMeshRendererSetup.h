#pragma once
#include "Core/VulkanRendererSetup.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	class VulkanMeshRendererSetup : public IVulkanRendererSetup
	{
	public:
		virtual void Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context) override;
		virtual void Teardown() override;

	private:
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_pipelines;
	};
}
