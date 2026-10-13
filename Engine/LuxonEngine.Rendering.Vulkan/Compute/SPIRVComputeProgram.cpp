#include "vulkan-pch.h"
#include "SPIRVComputeProgram.h"

namespace LuxonEngine::Rendering::Vulkan::Compute {
	SPIRVComputeProgram::SPIRVComputeProgram(const SPIRVShaderData& shader, const VkDevice device)
		:m_entryPoint(shader.entryPoint)
	{
		m_device = device;
		m_sampler = VK_NULL_HANDLE;

		if (shader.byteCode == nullptr)
			return;

		SpvReflectShaderModule reflectionModule;

		if (spvReflectCreateShaderModule(shader.byteCode->GetBufferSize(), shader.byteCode->GetBufferPointer(), &reflectionModule) != SPV_REFLECT_RESULT_SUCCESS)
			return;

		if (RemapDescriptorSets(reflectionModule)) {
			VkShaderModuleCreateInfo shaderModuleCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = spvReflectGetCodeSize(&reflectionModule),
				.pCode = spvReflectGetCode(&reflectionModule),
			};

			if (vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &m_computeShader) == VK_SUCCESS) {
				SPIRVVariableReflection::FillThreadGroupReflection(m_threadGroupReflection, &reflectionModule);
				SPIRVVariableReflection::AddShaderReflection(m_variableReflection, &reflectionModule);
			}
		}

		spvReflectDestroyShaderModule(&reflectionModule);

		m_stageInfo = VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = m_computeShader,
			.pName = m_entryPoint.c_str(),
		};
	}

	SPIRVComputeProgram::~SPIRVComputeProgram()
	{
		if (m_pipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

		for (auto& setLayout : m_descriptorSetLayouts)
			vkDestroyDescriptorSetLayout(m_device, setLayout, nullptr);

		if (m_sampler != VK_NULL_HANDLE)
			vkDestroySampler(m_device, m_sampler, nullptr);

		if (m_computeShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_computeShader, nullptr);
	}

	bool SPIRVComputeProgram::InitializePipelineLayout(std::string& error)
	{
		if (m_computeShader == VK_NULL_HANDLE) {
			error = "Compute stage is missing or failed to create";
			return false;
		}

		InitializeSampler();

		return SPIRVVariableReflection::CreatePipelineLayout(m_device, m_variableReflection, EntitySetIndex + 1, StageFlags, m_sampler,
			m_descriptorSetLayouts, m_pipelineLayout, error);
	}
}
