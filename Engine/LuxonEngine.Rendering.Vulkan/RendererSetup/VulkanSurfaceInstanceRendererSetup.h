#pragma once
#include "Core/VulkanRendererSetup.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	namespace MeshShading {
		class VulkanMeshPipelineModule;
	}

	class VulkanSurfaceInstanceRendererSetup : public IVulkanRendererSetup
	{
	public:
		virtual void Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context) override;
		virtual void Teardown() override;

	private:
		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> m_pipelines;
	};
}
