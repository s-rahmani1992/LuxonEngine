#include "vulkan-pch.h"
#include "VulkanTexture2DController.h"
#include "Core/Texture2D.h"
#include "VulkanUtilities.h"

const std::map<QuantumEngine::TextureFormat, VkFormat> QuantumEngine::Rendering::Vulkan::VulkanTexture2DController::s_texFormatMaps{
	{QuantumEngine::TextureFormat::Unknown, VK_FORMAT_UNDEFINED},
	{QuantumEngine::TextureFormat::RGBA32, VK_FORMAT_R8G8B8A8_UNORM},
	{QuantumEngine::TextureFormat::BGRA32, VK_FORMAT_B8G8R8A8_UNORM},
};

QuantumEngine::Rendering::Vulkan::VulkanTexture2DController::VulkanTexture2DController(const ref<Texture2D>& texture, const VkDevice device)
	:m_texture(texture), m_device(device)
{
}

bool QuantumEngine::Rendering::Vulkan::VulkanTexture2DController::Initialize(const VkPhysicalDeviceMemoryProperties& memoryProperties)
{
	VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = s_texFormatMaps.at(m_texture->GetFormat()),
		.extent = {m_texture->GetWidth(), m_texture->GetHeight(), 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if (vkCreateImage(m_device, &imageInfo, nullptr, &m_textureImage) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device, m_textureImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetMemoryTypeIndex(&memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryProperties);

	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_textureImageMemory) != VK_SUCCESS) {
		return false;
	}

	vkBindImageMemory(m_device, m_textureImage, m_textureImageMemory, 0);

	VkImageViewCreateInfo viewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = m_textureImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = s_texFormatMaps.at(m_texture->GetFormat()),
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};

	if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
		return false;;
	}

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanTexture2DController::CopyCommand(VkCommandBuffer commandBuffer, VkBuffer stageBuffer)
{
	VkImageMemoryBarrier imageCopyBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_textureImage,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageCopyBarrier);


	VkBufferImageCopy copyData{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
		.imageOffset = {0, 0, 0},
		.imageExtent = {m_texture->GetWidth(), m_texture->GetHeight(), 1},
	};

	vkCmdCopyBufferToImage(commandBuffer, stageBuffer, m_textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyData);

	VkImageMemoryBarrier imageEndCopyBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_textureImage,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageEndCopyBarrier);
}

void QuantumEngine::Rendering::Vulkan::VulkanTexture2DController::Release()
{
	vkDestroyImageView(m_device, m_imageView, nullptr);
	vkDestroyImage(m_device, m_textureImage, nullptr);
	vkFreeMemory(m_device, m_textureImageMemory, nullptr);
}