#include "vulkan-pch.h"
#include "VulkanMeshController.h"
#include "Core/Mesh.h"
#include "VulkanUtilities.h"
#include "VulkanDeviceManager.h"
#include "RayTracing/VulkanBLAS.h"
#include "Core/VulkanBufferFactory.h"

QuantumEngine::Rendering::Vulkan::VulkanMeshController::VulkanMeshController(const ref<Mesh>& mesh, const VkDevice device)
	: m_mesh(mesh), m_device(device)
{
}

bool QuantumEngine::Rendering::Vulkan::VulkanMeshController::Initialize(const ref<VulkanBufferFactory>& bufferFactory)
{
	VkMemoryAllocateFlagsInfo flags{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	};

	bufferFactory->CreateBuffer(sizeof(Vertex) * m_mesh->GetVertexCount(),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &flags, &m_vertexBuffer, &m_vertexBufferMemory);

	bufferFactory->CreateBuffer(sizeof(UInt32) * m_mesh->GetIndexCount(),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &flags, &m_indexBuffer, &m_indexBufferMemory);

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanMeshController::CopyCommand(VkCommandBuffer commandBuffer, VkBuffer stageBuffer, UInt32 offset)
{
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = offset;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(Vertex) * m_mesh->GetVertexCount();
	vkCmdCopyBuffer(commandBuffer, stageBuffer, m_vertexBuffer, 1, &copyRegion);
	copyRegion.srcOffset = offset + sizeof(Vertex) * m_mesh->GetVertexCount();
	copyRegion.size = sizeof(UInt32) * m_mesh->GetIndexCount();
	vkCmdCopyBuffer(commandBuffer, stageBuffer, m_indexBuffer, 1, &copyRegion);
}

void QuantumEngine::Rendering::Vulkan::VulkanMeshController::GetBLASBuildInfo(RayTracing::VulkanBLASBuildInfo* blasBuildInfo)
{
	VkBufferDeviceAddressInfo vbAddrInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = m_vertexBuffer,
	};
	VkDeviceAddress vertexAddress = vkGetBufferDeviceAddress(m_device, &vbAddrInfo);

	VkBufferDeviceAddressInfo ibAddrInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = m_indexBuffer,
	};
	VkDeviceAddress indexAddress = vkGetBufferDeviceAddress(m_device, &ibAddrInfo);

	VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	VkDeviceOrHostAddressConstKHR vertexDeviceAddress{};
	vertexDeviceAddress.deviceAddress = vertexAddress;
	triangles.vertexData = vertexDeviceAddress;
	triangles.vertexStride = sizeof(Vertex);
	triangles.maxVertex = m_mesh->GetVertexCount();
	triangles.indexType = VK_INDEX_TYPE_UINT32;
	VkDeviceOrHostAddressConstKHR indexDeviceAddress{};
	indexDeviceAddress.deviceAddress = indexAddress;
	triangles.indexData = indexDeviceAddress;
	VkDeviceOrHostAddressConstKHR transformDeviceAddress{};
	transformDeviceAddress.deviceAddress = 0;
	triangles.transformData = transformDeviceAddress;


	blasBuildInfo->geometryInfo = VkAccelerationStructureGeometryKHR{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry = VkAccelerationStructureGeometryDataKHR{
			.triangles = triangles,
		},
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	blasBuildInfo->buildInfo = VkAccelerationStructureBuildGeometryInfoKHR{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1,
		.pGeometries = &(blasBuildInfo->geometryInfo),
	};

	UInt32 primitiveCount = m_mesh->GetIndexCount() / 3;

	blasBuildInfo->sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	blasBuildInfo->primitiveCount = primitiveCount;
	auto getRTASGetSizePtr = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR");
	getRTASGetSizePtr(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&(blasBuildInfo->buildInfo),
		&primitiveCount,
		&(blasBuildInfo->sizeInfo)
	);
}

VkBuffer QuantumEngine::Rendering::Vulkan::VulkanMeshController::CreateVertexStorageBuffer()
{
	VkBufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = sizeof(Vertex) * m_mesh->GetVertexCount(); // same size as your index data
	info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer vertexStorageBuffer;
	vkCreateBuffer(m_device, &info, nullptr, &vertexStorageBuffer);
	vkBindBufferMemory(m_device, vertexStorageBuffer, m_vertexBufferMemory, 0);

	return vertexStorageBuffer;
}

VkBuffer QuantumEngine::Rendering::Vulkan::VulkanMeshController::CreateIndexStorageBuffer()
{
	VkBufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = sizeof(UInt32) * m_mesh->GetIndexCount(); // same size as your index data
	info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer indexStorageBuffer;
	vkCreateBuffer(m_device, &info, nullptr, &indexStorageBuffer);
	vkBindBufferMemory(m_device, indexStorageBuffer, m_indexBufferMemory, 0);

	return indexStorageBuffer;
}

void QuantumEngine::Rendering::Vulkan::VulkanMeshController::Release()
{
	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
	vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
}
