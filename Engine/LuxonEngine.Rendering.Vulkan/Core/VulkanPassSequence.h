#pragma once
#include "VulkanPass.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanTexture;

	class VulkanPassSequence
	{
	public:
		void AddPass(VulkanPass* pass);
		void SetBackBuffers(const std::vector<VkImage>& backBuffers);
		void Clear();
		bool Compile();
		void Execute(VkCommandBuffer commandBuffer, UInt32 backBufferIndex = 0);

	private:
		struct BarrierBatch
		{
			VkPipelineStageFlags srcStages = 0;
			VkPipelineStageFlags dstStages = 0;
			std::vector<VkImageMemoryBarrier> imageBarriers;
			std::vector<VkBufferMemoryBarrier> bufferBarriers;
		};

		struct EntryState
		{
			VulkanTexture* texture = nullptr;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		};

		std::vector<VulkanPass*> m_passes;
		std::vector<VkImage> m_backBuffers;

		std::vector<std::vector<BarrierBatch>> m_passBarriers;
		std::vector<BarrierBatch> m_endBarriers;
		BarrierBatch m_prologueBarriers;
		std::vector<EntryState> m_entryStates;

		bool m_compiled = false;
		bool m_prologueDone = false;
	};
}
