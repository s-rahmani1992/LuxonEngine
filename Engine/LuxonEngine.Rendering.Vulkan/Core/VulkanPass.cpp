#include "vulkan-pch.h"
#include "VulkanPass.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include <Core/Logger.h>

namespace LuxonEngine::Rendering::Vulkan {
	namespace {
		VkImageAspectFlags GetAspect(VkFormat format)
		{
			switch (format) {
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_X8_D24_UNORM_PACK32:
			case VK_FORMAT_D32_SFLOAT:
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			case VK_FORMAT_S8_UINT:
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			default:
				return VK_IMAGE_ASPECT_COLOR_BIT;
			}
		}
	}

	void VulkanPass::Use(VulkanTexture* texture, const VulkanResourceState& state)
	{
		if (texture == nullptr || texture->GetImage() == VK_NULL_HANDLE)
			return;

		AddUsage(VulkanResourceUsage{ .image = texture->GetImage(), .texture = texture, .aspect = GetAspect(texture->GetFormat()), .state = state });
	}

	void VulkanPass::Use(VulkanBuffer* buffer, const VulkanResourceState& state)
	{
		if (buffer == nullptr || buffer->GetBuffer() == VK_NULL_HANDLE)
			return;

		VulkanResourceState bufferState = state;
		bufferState.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		AddUsage(VulkanResourceUsage{ .buffer = buffer->GetBuffer(), .state = bufferState });
	}

	void VulkanPass::UseBackBuffer(const VulkanResourceState& state)
	{
		if (m_usesBackBuffer && (m_backBufferState.layout != state.layout || m_backBufferState.stages != state.stages || m_backBufferState.access != state.access)) {
			Logger::LogError("A pass declares two different states for the back buffer, the first one is used");
			return;
		}

		m_usesBackBuffer = true;
		m_backBufferState = state;
	}

	void VulkanPass::AddUsage(const VulkanResourceUsage& usage)
	{
		for (auto& existing : m_usages) {
			if (existing.image != usage.image || existing.buffer != usage.buffer)
				continue;

			if (existing.state.layout == usage.state.layout && existing.state.stages == usage.state.stages && existing.state.access == usage.state.access)
				return;

			if (existing.state.layout == usage.state.layout && IsReadOnlyState(existing.state) && IsReadOnlyState(usage.state)) {
				existing.state.stages |= usage.state.stages;
				existing.state.access |= usage.state.access;
			}
			else
				Logger::LogError("A pass declares two states that cannot be combined for the same resource, the first one is used");

			return;
		}

		m_usages.push_back(usage);
	}

	void VulkanPass::ClearUsages()
	{
		m_usages.clear();
		m_usesBackBuffer = false;
		m_backBufferState = VulkanResourceState();
	}
}
