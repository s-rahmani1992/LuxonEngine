#include "vulkan-pch.h"
#include "VulkanBuffer.h"

LuxonEngine::Rendering::Vulkan::VulkanBuffer::~VulkanBuffer()
{
	Release();
}

void LuxonEngine::Rendering::Vulkan::VulkanBuffer::Release()
{
	if (m_buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}

	if (m_memory != VK_NULL_HANDLE) {
		vkFreeMemory(m_device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
}
