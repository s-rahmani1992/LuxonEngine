#pragma once
#include "vulkan-pch.h"

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanPass
	{
	public:
		virtual ~VulkanPass() = default;

		virtual void Execute(VkCommandBuffer commandBuffer) = 0;
	};
}
