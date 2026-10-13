#pragma once
#include "vulkan-pch.h"
#include "VulkanBuffer.h"

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanBufferFactory {
	public:
		VulkanBufferFactory(const VkDevice device, const VkPhysicalDevice physicalDevice);

		template<typename T>
		Ptr<VulkanBuffer> CreateStructuredBuffer(const VulkanBufferDesc& desc, int count);

		template<typename T>
		Ptr<VulkanBuffer> CreateConstantBuffer(const VulkanBufferDesc& desc);

		bool CreateBuffer(UInt32 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffers, VkDeviceMemory* memory);
		bool CreateBuffer(UInt32 size, UInt32 amount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer* buffers, VkDeviceMemory* memory, UInt32* stride);
		bool CreateBuffer(UInt32 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, const void* pAllocNext, VkBuffer* buffers, VkDeviceMemory* memory);
		bool CreateImage(const VkImageCreateInfo* imageCreateInfo, VkMemoryPropertyFlags memoryPropertyFlags, VkImage* image, VkDeviceMemory* memory);
	private:
		Ptr<VulkanBuffer> CreateBuffer(const VulkanBufferDesc& desc, UInt32 size, VkBufferUsageFlags usage);

		static UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties);
		
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties m_physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties m_memoryProperties;

		VkBufferUsageFlags m_uniformFlag;
	};

	template<typename T>
	inline Ptr<VulkanBuffer> VulkanBufferFactory::CreateStructuredBuffer(const VulkanBufferDesc& desc, int count)
	{
		return CreateBuffer(desc, static_cast<UInt32>(sizeof(T)) * count, desc.usage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	template<typename T>
	inline Ptr<VulkanBuffer> VulkanBufferFactory::CreateConstantBuffer(const VulkanBufferDesc& desc)
	{
		const UInt32 alignment = static_cast<UInt32>(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
		const UInt32 alignedSize = (static_cast<UInt32>(sizeof(T)) + alignment - 1) / alignment * alignment;
		return CreateBuffer(desc, alignedSize, desc.usage | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	}
}