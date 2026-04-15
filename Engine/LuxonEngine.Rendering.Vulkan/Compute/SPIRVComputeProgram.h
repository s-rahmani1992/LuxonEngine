#pragma once
#include "vulkan-pch.h"
#include "Core/SPIRVShaderProgram.h"

namespace QuantumEngine::Rendering::Vulkan::Compute {
	class SPIRVComputeProgram : public SPIRVShaderProgram {
	public:
		SPIRVComputeProgram(Byte* bytecode, UInt64 codeSize, VkDevice device);
		~SPIRVComputeProgram();
		inline VkPipelineShaderStageCreateInfo& GetComputeStageInfo() { return m_computeStageInfo; }
		inline std::vector<VkDescriptorSetLayout>& GetDiscriptorLayouts() { return m_descriptorSetLayout; }
		inline VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
	private:
		Byte* m_bytecode;
		UInt64 m_codeSize; 
		
		VkShaderModule m_computeModule;
		VkPipelineShaderStageCreateInfo m_computeStageInfo;
		SpvReflectShaderModule m_reflectionModule;

		VkPipelineLayout m_pipelineLayout;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayout;
	};
}