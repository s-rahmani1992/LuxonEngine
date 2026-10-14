#pragma once
#include "vulkan-pch.h"

namespace LuxonEngine::Rendering::Vulkan {
	struct VulkanResourceState
	{
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags access = 0;
	};

	inline VkAccessFlags GetWriteAccess(const VulkanResourceState& state)
	{
		constexpr VkAccessFlags writeMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		return state.access & writeMask;
	}

	inline bool IsReadOnlyState(const VulkanResourceState& state)
	{
		return GetWriteAccess(state) == 0;
	}

	namespace VulkanStates {
		inline VulkanResourceState ColorAttachment()
		{
			return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };
		}

		inline VulkanResourceState DepthAttachment()
		{
			return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
		}

		inline VulkanResourceState Storage(VkPipelineStageFlags stages)
		{
			return { VK_IMAGE_LAYOUT_GENERAL, stages, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
		}

		inline VulkanResourceState Sampled(VkPipelineStageFlags stages)
		{
			return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, stages, VK_ACCESS_SHADER_READ_BIT };
		}

		inline VulkanResourceState VertexInput()
		{
			return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT };
		}
	}
}
