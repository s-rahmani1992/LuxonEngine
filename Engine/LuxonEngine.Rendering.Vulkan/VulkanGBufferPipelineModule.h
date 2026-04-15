#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine {
	class GameEntity;
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanMeshController;

	namespace Rasterization {
		class SPIRVRasterizationProgram;
	}

	struct VKEntityGPUData;

	struct GBufferEntityGPUData {
		ref<VulkanMeshController> meshController;
		UInt32 indexCount;
		UInt32 transformOffset;
	};

	class VulkanGBufferPipelineModule {
	public:
		VulkanGBufferPipelineModule();
		~VulkanGBufferPipelineModule();
		bool InitializePipeline(const std::vector<VKEntityGPUData>& entities, const ref<Rasterization::SPIRVRasterizationProgram>& gBufferProgram, UInt32 width, UInt32 height, VkImageView depthView);
		void WriteBuffer(const std::string& name, VkBuffer buffer, UInt32 stride);
		void RenderCommand(VkCommandBuffer commandBuffer);
		inline VkImageView GetPositionImageView() const { return m_positionImageView; }
		inline VkImageView GetNormalImageView() const { return m_normalImageView; }
		inline VkImageView GetMaskImageView() const { return m_maskImageView; }

		inline VkImage GetPositionImage() const { return m_positionImage; }
		inline VkImage GetNormalImage() const { return m_normalImage; }
		inline VkImage GetMaskImage() const { return m_maskImage; }
	private:
		bool CreateRenderPass();
		bool CreateFrameBuffers(UInt32 width, UInt32 height);
		bool CreateRasterPipeline();
		bool CreateDescriptorSets(const ref<Rasterization::SPIRVRasterizationProgram>& gBufferProgram);

		VkDevice m_device;
		VkPipeline m_gBufferPipeline;
		VkDescriptorPool m_descriptorPool;

		VkRenderPass m_renderPass;

		ref<Rasterization::SPIRVRasterizationProgram> m_gBufferProgram;
		VkImageView m_depthView;

		VkFormat m_positionFormat;
		VkImage m_positionImage;
		VkDeviceMemory m_positionImageMemory;
		VkImageView m_positionImageView;

		VkFormat m_normalFormat;
		VkImage m_normalImage;
		VkDeviceMemory m_normalImageMemory;
		VkImageView m_normalImageView;

		VkFormat m_maskFormat;
		VkImage m_maskImage;
		VkDeviceMemory m_maskImageMemory;
		VkImageView m_maskImageView;


		VkFramebuffer m_frameBuffer;

		std::vector<VkDescriptorSet> m_descriptorSets;

		std::vector<GBufferEntityGPUData> m_entities;

		VkClearValue m_clearValues[4];
		VkRenderPassBeginInfo m_renderPassInfo;

		std::vector<UInt32> m_offsets;
		UInt32 m_transformIndex = 0;
		UInt32 m_width;
		UInt32 m_height;
	};
}