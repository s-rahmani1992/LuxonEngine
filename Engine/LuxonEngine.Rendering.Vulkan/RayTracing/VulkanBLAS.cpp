#include "vulkan-pch.h"
#include "VulkanBLAS.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanBufferFactory.h"

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanBLAS::VulkanBLAS()
	:m_device(VulkanDeviceManager::Instance()->GetGraphicDevice())
{
	m_buildAccelerationStructurePtr = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR");
}

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanBLAS::~VulkanBLAS()
{
	auto vkDestroyAccelerationStructurePtr = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR");
	vkDestroyAccelerationStructurePtr(m_device, m_blas, nullptr);
	vkDestroyBuffer(m_device, m_blasBuffer, nullptr);
	vkFreeMemory(m_device, m_blasMemory, nullptr);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanBLAS::CreateCommand(VkCommandBuffer commandBuffer, VulkanBLASBuildInfo* buildInfo)
{
	auto bufferFactory = VulkanDeviceManager::Instance()->GetBufferFactory();

	VkMemoryAllocateFlagsInfo allocFlags{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	};

	bufferFactory->CreateBuffer(buildInfo->sizeInfo.accelerationStructureSize, 
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &m_blasBuffer, &m_blasMemory);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = m_blasBuffer,
		.size = buildInfo->sizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	};

	auto createAccelerationStructurePtr = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR");
	
	VkResult result = createAccelerationStructurePtr(m_device, &accelerationStructureCreateInfo, nullptr, &m_blas);

	buildInfo->buildInfo.dstAccelerationStructure = m_blas;

	VkAccelerationStructureBuildRangeInfoKHR range{
		.primitiveCount = buildInfo->primitiveCount,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};
	const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;

	VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = m_blas,
	};
	auto getAccelerationStructureDeviceAddressPtr = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR");
	m_blasAddress = getAccelerationStructureDeviceAddressPtr(m_device, &blasAddressInfo);

	m_buildAccelerationStructurePtr(commandBuffer, 1, &(buildInfo->buildInfo), &pRange);
}
