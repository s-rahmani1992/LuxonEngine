#include "vulkan-pch.h"
#include "VulkanTexture.h"

LuxonEngine::Rendering::Vulkan::VulkanTexture::~VulkanTexture()
{
	VulkanTexture::Release();
}

void LuxonEngine::Rendering::Vulkan::VulkanTexture::Release()
{
	if (m_view != VK_NULL_HANDLE) {
		vkDestroyImageView(m_device, m_view, nullptr);
		m_view = VK_NULL_HANDLE;
	}

	if (m_image != VK_NULL_HANDLE) {
		vkDestroyImage(m_device, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
	}

	if (m_memory != VK_NULL_HANDLE) {
		vkFreeMemory(m_device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
}
