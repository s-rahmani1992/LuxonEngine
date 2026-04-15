#include "vulkan-pch.h"
#include "VulkanUtilities.h"

UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties)
{
	for (UInt32 i = 0; i < memoryProperties->memoryTypeCount; i++) {
		if ((memoryRequirement->memoryTypeBits & (1 << i)) && (memoryProperties->memoryTypes[i].propertyFlags & targetFlags) == targetFlags) {
			return i;
		}
	}

	return UINT32_MAX;
}

bool CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, UInt32 size, UInt32 count, VkBuffer* buffer, VkDeviceMemory* memory)
{
	return false;
}
