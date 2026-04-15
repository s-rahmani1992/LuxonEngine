#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine::Rendering {
	class Material;
}

namespace QuantumEngine::Rendering::Vulkan::Rasterization {
	class SPIRVRasterizationProgram;

	class VulkanRasterizationMaterial {
	public:
		VulkanRasterizationMaterial(const ref<Material>& material, const VkDevice device);
		bool Initialize(const VkDescriptorPool pool);
		void BindValues(VkCommandBuffer commandBuffer);
		void BindDynamicValues(VkCommandBuffer commandBuffer, UInt32* offsets, UInt32 offsetCount);
		void WriteBuffer(const std::string name, const VkBuffer buffer, UInt32 stride);
		void SetImageView(const std::string& name, const VkImageView imageView);
	private:
		struct pushConstantData {
			UInt32 offset;
			Byte* location;
			UInt32 size;
		};

		struct DescriptorData {
			UInt32 setIndex;
			UInt32 binding;
			VkDescriptorType descriptorType;
		};

		ref<Material> m_material;
		VkDevice m_device;
		ref<SPIRVRasterizationProgram> m_program;
		VkPipelineLayout m_pipelineLayout;
		std::vector<pushConstantData> m_pushConstantValues; 
		std::vector<VkDescriptorSet> m_descriptorSets;
		std::vector<DescriptorData> m_descriptorData;
	};
}