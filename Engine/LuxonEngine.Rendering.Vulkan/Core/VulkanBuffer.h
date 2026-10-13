#pragma once
#include "vulkan-pch.h"

namespace LuxonEngine::Rendering::Vulkan {
	enum class VulkanMemoryType
	{
		GPUOnly,
		Upload,
	};

	struct VulkanBufferDesc
	{
		VulkanMemoryType memoryType = VulkanMemoryType::GPUOnly;
		VkBufferUsageFlags usage = 0;
	};

	class VulkanBuffer
	{
	public:
		VulkanBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory memory)
			: m_device(device), m_buffer(buffer), m_memory(memory) {}
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		void Release();

		inline VkBuffer GetBuffer() const { return m_buffer; }
		inline VkDeviceMemory GetMemory() const { return m_memory; }

	private:
		VkDevice m_device;
		VkBuffer m_buffer;
		VkDeviceMemory m_memory;
	};
}
