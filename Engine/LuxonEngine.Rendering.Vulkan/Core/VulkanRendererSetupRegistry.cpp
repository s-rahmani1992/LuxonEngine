#include "vulkan-pch.h"
#include "VulkanRendererSetupRegistry.h"
#include <Rendering/Renderer.h>

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanRendererSetupRegistry::Register(RendererTypeId typeId, Ptr<IVulkanRendererSetup> setup)
	{
		if (setup == nullptr)
			return;

		for (auto& entry : m_entries) {
			if (entry.typeId == typeId) {
				entry.setup = std::move(setup);
				return;
			}
		}

		m_entries.push_back(Entry{ typeId, std::move(setup) });
	}

	void VulkanRendererSetupRegistry::Setup(const std::vector<VulkanRendererEntity>& entities, VulkanPassSetupContext& context)
	{
		std::vector<VulkanRendererEntity> group;

		for (auto& entry : m_entries) {
			group.clear();

			for (auto& rendererEntity : entities) {
				if (rendererEntity.renderer != nullptr && rendererEntity.renderer->GetTypeId() == entry.typeId)
					group.push_back(rendererEntity);
			}

			if (group.empty() == false)
				entry.setup->Setup(group, context);
		}
	}

	void VulkanRendererSetupRegistry::Update()
	{
		for (auto& entry : m_entries)
			entry.setup->Update();
	}

	void VulkanRendererSetupRegistry::Teardown()
	{
		for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it)
			it->setup->Teardown();
	}
}
