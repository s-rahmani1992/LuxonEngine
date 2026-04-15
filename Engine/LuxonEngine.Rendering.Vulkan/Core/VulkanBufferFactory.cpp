#include "vulkan-pch.h"
#include "VulkanBufferFactory.h"
#include "VulkanUtilities.h"

QuantumEngine::Rendering::Vulkan::VulkanBufferFactory::VulkanBufferFactory(const VkDevice device, const VkPhysicalDevice physicalDevice)
	:m_device(device), m_physicalDevice(physicalDevice),
	m_uniformFlag(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
{
	vkGetPhysicalDeviceProperties(physicalDevice, &m_physicalDeviceProperties);

	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
}

bool QuantumEngine::Rendering::Vulkan::VulkanBufferFactory::CreateBuffer(UInt32 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffers, VkDeviceMemory* memory)
{
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	if (vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, buffers) != VK_SUCCESS)
		return false;

	VkMemoryRequirements bufferMemoryRequirement;
	vkGetBufferMemoryRequirements(m_device, *buffers, &bufferMemoryRequirement);

	VkMemoryAllocateInfo stageAllocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = bufferMemoryRequirement.size,
		.memoryTypeIndex = GetMemoryTypeIndex(&bufferMemoryRequirement, memoryPropertyFlags, &m_memoryProperties),
	};

	if (vkAllocateMemory(m_device, &stageAllocInfo, nullptr, memory) != VK_SUCCESS) {
		return false;
	}

	vkBindBufferMemory(m_device, *buffers, *memory, 0);

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanBufferFactory::CreateBuffer(UInt32 size, UInt32 amount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffer, VkDeviceMemory* memory, UInt32* stride)
{
	UInt32 alignment = 4;;

	if ((usageFlags & m_uniformFlag) > 0)
		alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

	if ((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) > 0)
		alignment = m_physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;

	UInt32 objectSize = ALIGN(size, alignment);
	*stride = objectSize;

	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = objectSize * amount,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	if (vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS)
		return false;

	VkMemoryRequirements bufferMemoryRequirement;
	vkGetBufferMemoryRequirements(m_device, *buffer, &bufferMemoryRequirement);

	VkMemoryAllocateInfo stageAllocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = bufferMemoryRequirement.size,
		.memoryTypeIndex = GetMemoryTypeIndex(&bufferMemoryRequirement, memoryPropertyFlags, &m_memoryProperties),
	};

	if (vkAllocateMemory(m_device, &stageAllocInfo, nullptr, memory) != VK_SUCCESS) {
		return false;
	}

	vkBindBufferMemory(m_device, *buffer, *memory, 0);

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanBufferFactory::CreateBuffer(UInt32 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, const void* pAllocNext, VkBuffer* buffer, VkDeviceMemory* memory)
{
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, buffer);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(m_device, *buffer, &memReq);

	VkMemoryAllocateInfo scratchAlloc{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = pAllocNext,
		.allocationSize = memReq.size,
		.memoryTypeIndex = GetMemoryTypeIndex(&memReq, memoryPropertyFlags, &m_memoryProperties),
	};

	vkAllocateMemory(m_device, &scratchAlloc, nullptr, memory);
	vkBindBufferMemory(m_device, *buffer, *memory, 0);
	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanBufferFactory::CreateImage(const VkImageCreateInfo* imageCreateInfo, VkMemoryPropertyFlags memoryPropertyFlags, VkImage* image, VkDeviceMemory* memory)
{
	vkCreateImage(m_device, imageCreateInfo, nullptr, image);

	// Allocate memory
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(m_device, *image, &memReq);

	VkMemoryAllocateInfo allocInfo{
	.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	.allocationSize = memReq.size,
	.memoryTypeIndex = GetMemoryTypeIndex(&memReq, memoryPropertyFlags, &m_memoryProperties),
	};

	vkAllocateMemory(m_device, &allocInfo, nullptr, memory);
	vkBindImageMemory(m_device, *image, *memory, 0);
	return true;
}

UInt32 QuantumEngine::Rendering::Vulkan::VulkanBufferFactory::GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties)
{
	for (UInt32 i = 0; i < memoryProperties->memoryTypeCount; i++) {
		if ((memoryRequirement->memoryTypeBits & (1 << i)) && (memoryProperties->memoryTypes[i].propertyFlags & targetFlags) == targetFlags) {
			return i;
		}
	}

	return UINT32_MAX;
}
