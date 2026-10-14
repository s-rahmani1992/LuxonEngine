#pragma once
#include "VulkanPass.h"
#include <array>
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanRenderTexture;
	class VulkanDepthTexture;

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	namespace MeshShading {
		class VulkanMeshPipelineModule;
	}

	struct VulkanRenderPassParams
	{
		VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		std::vector<std::array<float, 4>> clearColors;

		VulkanDepthTexture* depthTarget = nullptr;
		VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		float clearDepth = 1.0f;
		UInt8 clearStencil = 0;
	};

	class VulkanRenderPass : public VulkanPass
	{
	public:
		VulkanRenderPass() = default;
		~VulkanRenderPass() override;

		VulkanRenderPass(const VulkanRenderPass&) = delete;
		VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

		bool Initialize(VkDevice device, VkFormat format, VkExtent2D extent, const std::vector<VkImageView>& imageViews, const VulkanRenderPassParams& params = {});
		bool Initialize(VkDevice device, const std::vector<VulkanRenderTexture*>& renderTargets, const VulkanRenderPassParams& params = {});

		void AddPipeline(Rasterization::VulkanRasterizationPipelineModule* pipeline);
		void AddPipeline(MeshShading::VulkanMeshPipelineModule* pipeline);

		void SetImageIndex(UInt32 imageIndex);

		inline VkRenderPass GetRenderPass() const { return m_renderPass; }

		virtual void Execute(VkCommandBuffer commandBuffer) override;

	private:
		void Reset();

		bool CreateRenderPass(const std::vector<VkFormat>& colorFormats, VkImageLayout colorFinalLayout, const VulkanRenderPassParams& params);
		bool CreateFramebuffer(const std::vector<VkImageView>& colorViews, const VulkanRenderPassParams& params);
		void BuildClearValues(size_t colorCount, const VulkanRenderPassParams& params);

		VkDevice m_device = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> m_framebuffers;
		std::vector<VkClearValue> m_clearValues;
		VkExtent2D m_extent = {};
		UInt32 m_imageIndex = 0;

		std::vector<Rasterization::VulkanRasterizationPipelineModule*> m_pipelines;
		std::vector<MeshShading::VulkanMeshPipelineModule*> m_meshPipelines;
	};
}
