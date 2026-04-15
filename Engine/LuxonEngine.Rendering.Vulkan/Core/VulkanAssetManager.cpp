#include "vulkan-pch.h"
#include "VulkanAssetManager.h"
#include "Core/Mesh.h"
#include "VulkanMeshController.h"
#include <map>
#include "VulkanUtilities.h"
#include "Core/Texture2D.h"
#include "VulkanTexture2DController.h"
#include "VulkanBufferFactory.h"
#include "VulkanDeviceManager.h"

QuantumEngine::Rendering::Vulkan::VulkanAssetManager::VulkanAssetManager(const VkDevice device, VkPhysicalDevice physicalDevice)
	: m_device(device), m_physicalDevice(physicalDevice),
	m_bufferFactory(VulkanDeviceManager::Instance()->GetBufferFactory())
{
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
}

QuantumEngine::Rendering::Vulkan::VulkanAssetManager::~VulkanAssetManager()
{
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::VulkanAssetManager::Initializes(UInt32 familyIndex)
{
	// Create Queue, Command Pool and Command Buffer
	vkGetDeviceQueue(m_device, familyIndex, 0, &m_graphicsQueue);


	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = familyIndex,
	};

	if(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		return false;

	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	if(vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer) != VK_SUCCESS)
		return false;

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UploadMeshToGPU(const ref<Mesh>& mesh)
{
	UploadMeshesToGPU({ mesh });
}

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UploadTextureToGPU(const ref<Texture2D>& texture)
{
	if(m_texturePairs.find(texture) != m_texturePairs.end())
		return;

	ref<VulkanTexture2DController> gpuTexture = std::make_shared<VulkanTexture2DController>(texture, m_device);

	if (gpuTexture->Initialize(m_memoryProperties) == false)
		return;

	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;

	m_bufferFactory->CreateBuffer(texture->GetTotalSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr, &stageBuffer, &stageBufferMemory);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	void* data;
	vkMapMemory(m_device, stageBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
	std::memcpy(data, texture->GetData(), texture->GetTotalSize());

	gpuTexture->CopyCommand(m_commandBuffer, stageBuffer);

	vkUnmapMemory(m_device, stageBufferMemory);
	vkEndCommandBuffer(m_commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);
	texture->SetGPUHandle(gpuTexture);
	m_texturePairs.emplace(texture, gpuTexture);

	vkDestroyBuffer(m_device, stageBuffer, nullptr);
	vkFreeMemory(m_device, stageBufferMemory, nullptr);
}

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UploadMeshesToGPU(const std::vector<ref<Mesh>>& meshes)
{
	std::map<ref<Mesh>, ref<VulkanMeshController>> meshPairs;
	UInt32 totalVBSize = 0;

	for(auto& mesh : meshes)
	{
		if(mesh->IsUploadedToGPU())
			continue;

		auto meshPairIt = meshPairs.emplace(mesh, nullptr);

		if(meshPairIt.second == false)
			continue;

		ref<VulkanMeshController> meshController = std::make_shared<VulkanMeshController>(mesh, m_device);
		if (meshController->Initialize(m_bufferFactory)) {
			(*(meshPairIt.first)).second = meshController;

			totalVBSize += mesh->GetTotalSize();
		}
	}

	VkBufferCreateInfo stageBufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = totalVBSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;
	m_bufferFactory->CreateBuffer(totalVBSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr, &stageBuffer, &stageBufferMemory);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	void* data;
	vkMapMemory(m_device, stageBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
	Byte* dataPtr = (Byte*)data;
	UInt32 offset = 0;

	for(auto& meshPair : meshPairs)
	{
		meshPair.first->CopyVertexData(dataPtr);
		dataPtr += sizeof(Vertex) * meshPair.first->GetVertexCount();
		meshPair.first->CopyIndexData(dataPtr);
		dataPtr += sizeof(UInt32) * meshPair.first->GetIndexCount();
		meshPair.second->CopyCommand(m_commandBuffer, stageBuffer, offset);
		offset += meshPair.first->GetTotalSize();
	}

	vkUnmapMemory(m_device, stageBufferMemory);

	vkEndCommandBuffer(m_commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	for(auto & meshPair : meshPairs)
	{
		meshPair.first->SetGPUHandle(meshPair.second);
	}

	m_meshPairs.insert(meshPairs.begin(), meshPairs.end());

	vkDestroyBuffer(m_device, stageBuffer, nullptr);
	vkFreeMemory(m_device, stageBufferMemory, nullptr);
}

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UnloadAssets()
{
	for(auto& [texture, gpuTexture] : m_texturePairs)
	{
		texture->Release();
		gpuTexture->Release();
	}

	for(auto& [mesh, gpuMesh] : m_meshPairs)
	{
		mesh->Release();
		gpuMesh->Release();
	}
}
