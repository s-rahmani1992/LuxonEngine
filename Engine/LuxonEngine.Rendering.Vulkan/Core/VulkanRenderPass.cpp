#include "vulkan-pch.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderTexture.h"
#include "VulkanDepthTexture.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "Mesh/VulkanMeshPipelineModule.h"

namespace LuxonEngine::Rendering::Vulkan {
	namespace {
		bool HasStencil(VkFormat format)
		{
			return format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_S8_UINT;
		}
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		Reset();
	}

	void VulkanRenderPass::Reset()
	{
		for (VkFramebuffer framebuffer : m_framebuffers)
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);

		if (m_renderPass != VK_NULL_HANDLE)
			vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		m_framebuffers.clear();
		m_clearValues.clear();
		m_renderPass = VK_NULL_HANDLE;
		m_device = VK_NULL_HANDLE;
		m_extent = {};
		m_imageIndex = 0;
	}

	bool VulkanRenderPass::Initialize(VkDevice device, VkFormat format, VkExtent2D extent, const std::vector<VkImageView>& imageViews, const VulkanRenderPassParams& params)
	{
		Reset();
		if (device == VK_NULL_HANDLE || imageViews.empty())
			return false;

		m_device = device;
		m_extent = extent;

		if (CreateRenderPass({ format }, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, params) == false) {
			Reset();
			return false;
		}

		for (VkImageView imageView : imageViews) {
			if (CreateFramebuffer({ imageView }, params) == false) {
				Reset();
				return false;
			}
		}

		BuildClearValues(1, params);
		return true;
	}

	bool VulkanRenderPass::Initialize(VkDevice device, const std::vector<VulkanRenderTexture*>& renderTargets, const VulkanRenderPassParams& params)
	{
		Reset();
		if (device == VK_NULL_HANDLE)
			return false;

		std::vector<VkFormat> colorFormats;
		std::vector<VkImageView> colorViews;

		for (VulkanRenderTexture* target : renderTargets) {
			if (target == nullptr)
				continue;

			if (colorViews.empty())
				m_extent = VkExtent2D{ target->GetWidth(), target->GetHeight() };

			colorFormats.push_back(target->GetFormat());
			colorViews.push_back(target->GetAttachmentView());
		}

		if (colorViews.empty()) {
			if (params.depthTarget == nullptr)
				return false;

			m_extent = VkExtent2D{ params.depthTarget->GetWidth(), params.depthTarget->GetHeight() };
		}

		m_device = device;

		if (CreateRenderPass(colorFormats, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, params) == false || CreateFramebuffer(colorViews, params) == false) {
			Reset();
			return false;
		}

		BuildClearValues(colorViews.size(), params);
		return true;
	}

	bool VulkanRenderPass::CreateRenderPass(const std::vector<VkFormat>& colorFormats, VkImageLayout colorFinalLayout, const VulkanRenderPassParams& params)
	{
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorRefs;

		for (VkFormat format : colorFormats) {
			colorRefs.push_back(VkAttachmentReference{ .attachment = (UInt32)attachments.size(), .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			attachments.push_back(VkAttachmentDescription{
				.format = format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = params.colorLoadOp,
				.storeOp = params.colorStoreOp,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = params.colorLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD ? colorFinalLayout : VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = colorFinalLayout,
			});
		}

		VkAttachmentReference depthRef{ .attachment = (UInt32)attachments.size(), .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		if (params.depthTarget != nullptr) {
			const bool hasStencil = HasStencil(params.depthTarget->GetFormat());

			attachments.push_back(VkAttachmentDescription{
				.format = params.depthTarget->GetFormat(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = params.depthLoadOp,
				.storeOp = params.depthStoreOp,
				.stencilLoadOp = hasStencil ? params.depthLoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = hasStencil ? params.depthStoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = params.depthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			});
		}

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = (UInt32)colorRefs.size(),
			.pColorAttachments = colorRefs.data(),
			.pDepthStencilAttachment = params.depthTarget != nullptr ? &depthRef : nullptr,
		};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		VkRenderPassCreateInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = (UInt32)attachments.size(),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		return vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS;
	}

	bool VulkanRenderPass::CreateFramebuffer(const std::vector<VkImageView>& colorViews, const VulkanRenderPassParams& params)
	{
		std::vector<VkImageView> views = colorViews;

		if (params.depthTarget != nullptr)
			views.push_back(params.depthTarget->GetAttachmentView());

		VkFramebufferCreateInfo framebufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_renderPass,
			.attachmentCount = (UInt32)views.size(),
			.pAttachments = views.data(),
			.width = m_extent.width,
			.height = m_extent.height,
			.layers = 1,
		};

		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
			return false;

		m_framebuffers.push_back(framebuffer);
		return true;
	}

	void VulkanRenderPass::BuildClearValues(size_t colorCount, const VulkanRenderPassParams& params)
	{
		for (size_t i = 0; i < colorCount; i++) {
			const std::array<float, 4> color = i < params.clearColors.size() ? params.clearColors[i] : std::array<float, 4>{};

			VkClearValue clearValue{};
			clearValue.color = { { color[0], color[1], color[2], color[3] } };
			m_clearValues.push_back(clearValue);
		}

		if (params.depthTarget != nullptr) {
			VkClearValue clearValue{};
			clearValue.depthStencil = { params.clearDepth, params.clearStencil };
			m_clearValues.push_back(clearValue);
		}
	}

	void VulkanRenderPass::AddPipeline(Rasterization::VulkanRasterizationPipelineModule* pipeline)
	{
		if (pipeline != nullptr)
			m_pipelines.push_back(pipeline);
	}

	void VulkanRenderPass::AddPipeline(MeshShading::VulkanMeshPipelineModule* pipeline)
	{
		if (pipeline != nullptr)
			m_meshPipelines.push_back(pipeline);
	}

	void VulkanRenderPass::SetImageIndex(UInt32 imageIndex)
	{
		m_imageIndex = imageIndex;
	}

	void VulkanRenderPass::Execute(VkCommandBuffer commandBuffer)
	{
		if (m_renderPass == VK_NULL_HANDLE || m_imageIndex >= m_framebuffers.size())
			return;

		VkRenderPassBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_renderPass,
			.framebuffer = m_framebuffers[m_imageIndex],
			.renderArea = { .offset = { 0, 0 }, .extent = m_extent },
			.clearValueCount = (UInt32)m_clearValues.size(),
			.pClearValues = m_clearValues.data(),
		};

		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{
			.x = 0.0f, .y = 0.0f,
			.width = (float)m_extent.width, .height = (float)m_extent.height,
			.minDepth = 0.0f, .maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{ .offset = { 0, 0 }, .extent = m_extent };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (auto* pipeline : m_pipelines)
			pipeline->Draw(commandBuffer);

		for (auto* pipeline : m_meshPipelines)
			pipeline->Dispatch(commandBuffer);

		vkCmdEndRenderPass(commandBuffer);
	}
}
