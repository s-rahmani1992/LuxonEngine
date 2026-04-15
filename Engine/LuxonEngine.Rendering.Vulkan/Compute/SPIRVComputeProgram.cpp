#include "vulkan-pch.h"
#include "SPIRVComputeProgram.h"

QuantumEngine::Rendering::Vulkan::Compute::SPIRVComputeProgram::SPIRVComputeProgram(Byte* bytecode, UInt64 codeSize, VkDevice device)
	:m_codeSize(codeSize)
{
	m_device = device;
	m_bytecode = new Byte[m_codeSize];
	std::memcpy(m_bytecode, bytecode, m_codeSize);

	VkShaderModuleCreateInfo shaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = codeSize,
		.pCode = reinterpret_cast<UInt32*>(m_bytecode),
	};

	vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &m_computeModule);

	spvReflectCreateShaderModule(m_codeSize, m_bytecode, &m_reflectionModule);
	m_reflection.AddShaderReflection(&m_reflectionModule);

	m_reflection.Initializes();
	InitializeSampler();

	m_computeStageInfo = VkPipelineShaderStageCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = m_computeModule,
		.pName = m_reflectionModule.entry_point_name,
	};

	m_descriptorSetLayout.resize(m_reflection.GetDescriptorLayoutCount());
	m_reflection.CreatePipelineLayout(m_device, VK_SHADER_STAGE_COMPUTE_BIT, m_sampler, &m_pipelineLayout, m_descriptorSetLayout.data());
}

QuantumEngine::Rendering::Vulkan::Compute::SPIRVComputeProgram::~SPIRVComputeProgram()
{
	vkDestroySampler(m_device, m_sampler, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	for (auto& descriptorLayout : m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(m_device, descriptorLayout, nullptr);

	delete[] m_bytecode;
}
