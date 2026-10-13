#pragma once
#include "VulkanTexture.h"

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanDepthTexture : public VulkanTexture
	{
	public:
		VulkanDepthTexture(VkDevice device, VkImage image, VkDeviceMemory memory, VkImageView view, UInt32 width, UInt32 height, VkFormat format, VkImageView attachmentView)
			: VulkanTexture(device, image, memory, view, width, height, format), m_attachmentView(attachmentView) {}
		~VulkanDepthTexture() override { VulkanDepthTexture::Release(); }

		virtual void Release() override
		{
			if (m_attachmentView != VK_NULL_HANDLE) {
				vkDestroyImageView(m_device, m_attachmentView, nullptr);
				m_attachmentView = VK_NULL_HANDLE;
			}
			VulkanTexture::Release();
		}

		inline VkImageView GetAttachmentView() const { return m_attachmentView; }

	private:
		VkImageView m_attachmentView;
	};
}
