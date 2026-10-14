#pragma once
#include "VulkanRendererSetup.h"
#include <Rendering/RendererType.h>
#include <utility>
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanRendererSetupRegistry {
	public:
		template<typename RendererType, typename SetupType, typename... Args>
		void Register(Args&&... args)
		{
			Register(RendererType::StaticTypeId(), std::make_unique<SetupType>(std::forward<Args>(args)...));
		}

		void Register(RendererTypeId typeId, Ptr<IVulkanRendererSetup> setup);
		void Setup(const std::vector<VulkanRendererEntity>& entities, VulkanPassSetupContext& context);
		void Update();
		void Teardown();

	private:
		struct Entry {
			RendererTypeId typeId = 0;
			Ptr<IVulkanRendererSetup> setup;
		};

		std::vector<Entry> m_entries;
	};
}
