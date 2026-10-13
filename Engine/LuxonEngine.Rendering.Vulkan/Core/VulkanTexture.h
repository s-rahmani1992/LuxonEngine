#pragma once
#include "vulkan-pch.h"

namespace LuxonEngine::Rendering::Vulkan {
	struct VulkanTextureDesc
	{
		UInt32 width = 1;
		UInt32 height = 1;
		UInt32 mipLevels = 1;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags usage = 0;
	};

	class VulkanTexture
	{
	public:
		VulkanTexture(VkDevice device, VkImage image, VkDeviceMemory memory, VkImageView view, UInt32 width, UInt32 height, VkFormat format)
			: m_device(device), m_image(image), m_memory(memory), m_view(view), m_width(width), m_height(height), m_format(format) {}
		virtual ~VulkanTexture();

		VulkanTexture(const VulkanTexture&) = delete;
		VulkanTexture& operator=(const VulkanTexture&) = delete;

		virtual void Release();

		inline VkImage GetImage() const { return m_image; }
		inline VkDeviceMemory GetMemory() const { return m_memory; }
		inline VkImageView GetView() const { return m_view; }
		inline UInt32 GetWidth() const { return m_width; }
		inline UInt32 GetHeight() const { return m_height; }
		inline VkFormat GetFormat() const { return m_format; }

		// Layout as last recorded by the user of this texture. The texture never issues barriers itself.
		inline VkImageLayout GetLayout() const { return m_layout; }
		inline void SetLayout(VkImageLayout layout) { m_layout = layout; }

	protected:
		VkDevice m_device;

	private:
		VkImage m_image;
		VkDeviceMemory m_memory;
		VkImageView m_view;
		UInt32 m_width = 0;
		UInt32 m_height = 0;
		VkFormat m_format = VK_FORMAT_UNDEFINED;
		VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};
}
