#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine {
	class GameEntity;
	class Mesh;

	namespace Rendering::Vulkan {
		class VulkanMeshController;
	}
}

namespace QuantumEngine::Rendering::Vulkan::Rasterization {
	class VulkanRasterizationMaterial;
	class SPIRVRasterizationProgram;

	class VulkanRasterizationPipelineModule
	{
	public:
		VulkanRasterizationPipelineModule(const VkDevice device);
		~VulkanRasterizationPipelineModule();
		void RenderCommand(VkCommandBuffer commandBuffer);
		bool Initialize(const ref<GameEntity>& entity, ref<VulkanRasterizationMaterial> material, const VkRenderPass m_renderPass);
		void SetDescriptorOffset(const std::string& name, UInt32 offset);
	private:
		static VkVertexInputBindingDescription s_bindingDescriptions;
		static VkVertexInputAttributeDescription s_attributeDescriptions[3];
		static VkPipelineVertexInputStateCreateInfo s_vertexInputInfo;

		VkDevice m_device; 
		VkPipeline m_graphicsPipeline;
		ref<Mesh> m_mesh;
		ref<VulkanMeshController> m_meshController;
		ref<VulkanRasterizationMaterial> m_material;
		ref<SPIRVRasterizationProgram> m_program;
		std::vector<UInt32> m_offset;
	};
}