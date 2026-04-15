#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanBufferFactory {
	public:
		VulkanBufferFactory(const VkDevice device, const VkPhysicalDevice physicalDevice);
		bool CreateBuffer(UInt32 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffers, VkDeviceMemory* memory);
		bool CreateBuffer(UInt32 size, UInt32 amount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffers, VkDeviceMemory* memory, UInt32* stride);
		bool CreateBuffer(UInt32 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, const void* pAllocNext, VkBuffer* buffers, VkDeviceMemory* memory);
		bool CreateImage(const VkImageCreateInfo* imageCreateInfo, VkMemoryPropertyFlags memoryPropertyFlags, VkImage* image, VkDeviceMemory* memory);
	private:
		static UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties);
		
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties m_physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties m_memoryProperties;

		VkBufferUsageFlags m_uniformFlag;
	};
}