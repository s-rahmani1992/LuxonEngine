#pragma once
#include "vulkan-pch.h"
#include "VulkanResourceState.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanBuffer;
	class VulkanTexture;

	struct VulkanResourceUsage
	{
		VkImage image = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		VulkanTexture* texture = nullptr;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VulkanResourceState state;
	};

	class VulkanPass
	{
	public:
		virtual ~VulkanPass() = default;

		void Use(VulkanTexture* texture, const VulkanResourceState& state);
		void Use(VulkanBuffer* buffer, const VulkanResourceState& state);

		void UseBackBuffer(const VulkanResourceState& state);

		inline const std::vector<VulkanResourceUsage>& GetUsages() const { return m_usages; }
		inline bool UsesBackBuffer() const { return m_usesBackBuffer; }
		inline const VulkanResourceState& GetBackBufferState() const { return m_backBufferState; }

		virtual void Execute(VkCommandBuffer commandBuffer) = 0;

	protected:
		void ClearUsages();

	private:
		void AddUsage(const VulkanResourceUsage& usage);

		std::vector<VulkanResourceUsage> m_usages;
		bool m_usesBackBuffer = false;
		VulkanResourceState m_backBufferState;
	};
}
