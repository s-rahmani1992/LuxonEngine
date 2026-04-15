#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	struct VulkanBLASBuildInfo {
		VkAccelerationStructureGeometryKHR geometryInfo;
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
		UInt32 primitiveCount;
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo;
	};

	class VulkanBLAS {
	public:
		VulkanBLAS();
		~VulkanBLAS();
		void CreateCommand(VkCommandBuffer commandBuffer, VulkanBLASBuildInfo* buildInfo);
		VkDeviceAddress GetBLASAddress() const { return m_blasAddress; }
	private:
		VkDevice m_device; 
		VkAccelerationStructureKHR m_blas;
		VkDeviceAddress m_blasAddress;
		VkBuffer m_blasBuffer;
		VkDeviceMemory m_blasMemory;
		PFN_vkCmdBuildAccelerationStructuresKHR m_buildAccelerationStructurePtr;
	};
}