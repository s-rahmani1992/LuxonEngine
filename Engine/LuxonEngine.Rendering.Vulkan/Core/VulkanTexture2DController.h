#pragma once
#include "vulkan-pch.h"
#include "Rendering/GPUTexture2DController.h"
#include <map>

namespace QuantumEngine {
	class Texture2D;
	enum class TextureFormat;
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanTexture2DController : public GPUTexture2DController {
	public:
		VulkanTexture2DController(const ref<Texture2D>& texture, const VkDevice device);
		bool Initialize(const VkPhysicalDeviceMemoryProperties& memoryProperties);
		void CopyCommand(VkCommandBuffer commandBuffer, VkBuffer stageBuffer);
		inline VkImageView GetImageView() const { return m_imageView; }
		void Release();
	private:
		const static std::map<TextureFormat, VkFormat> s_texFormatMaps;
		ref<Texture2D> m_texture;
		VkDevice m_device;

		VkImage m_textureImage;
		VkDeviceMemory m_textureImageMemory;
		VkImageView m_imageView;
	};
}