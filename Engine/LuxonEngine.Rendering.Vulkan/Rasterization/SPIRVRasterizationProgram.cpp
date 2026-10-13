#include "vulkan-pch.h"
#include "SPIRVRasterizationProgram.h"

namespace LuxonEngine::Rendering::Vulkan::Rasterization {
	SPIRVRasterizationProgram::SPIRVRasterizationProgram(const std::vector<SPIRVShaderData>& shaders, const VkDevice device)
	{
		m_device = device;
		m_sampler = VK_NULL_HANDLE;

		for (auto& shader : shaders) {
			if (shader.byteCode == nullptr)
				continue;

			if (shader.shaderType == VK_SHADER_STAGE_VERTEX_BIT)
				m_vertexShader = CreateStage(shader, m_vertexEntryPoint);
			else if (shader.shaderType == VK_SHADER_STAGE_GEOMETRY_BIT)
				m_geometryShader = CreateStage(shader, m_geometryEntryPoint);
			else if (shader.shaderType == VK_SHADER_STAGE_FRAGMENT_BIT)
				m_pixelShader = CreateStage(shader, m_pixelEntryPoint);
		}

		auto addStageInfo = [this](VkShaderStageFlagBits stage, VkShaderModule module, const std::string& entryPoint) {
			if (module == VK_NULL_HANDLE)
				return;

			m_stageInfos.push_back(VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = stage,
				.module = module,
				.pName = entryPoint.c_str(),
				});
		};

		addStageInfo(VK_SHADER_STAGE_VERTEX_BIT, m_vertexShader, m_vertexEntryPoint);
		addStageInfo(VK_SHADER_STAGE_GEOMETRY_BIT, m_geometryShader, m_geometryEntryPoint);
		addStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, m_pixelShader, m_pixelEntryPoint);
	}

	SPIRVRasterizationProgram::~SPIRVRasterizationProgram()
	{
		if (m_pipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

		for (auto& setLayout : m_descriptorSetLayouts)
			vkDestroyDescriptorSetLayout(m_device, setLayout, nullptr);

		if (m_sampler != VK_NULL_HANDLE)
			vkDestroySampler(m_device, m_sampler, nullptr);

		if (m_vertexShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_vertexShader, nullptr);

		if (m_geometryShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_geometryShader, nullptr);

		if (m_pixelShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_pixelShader, nullptr);
	}

	VkShaderModule SPIRVRasterizationProgram::CreateStage(const SPIRVShaderData& shader, std::string& entryPoint)
	{
		entryPoint = shader.entryPoint;

		SpvReflectShaderModule reflectionModule;

		if (spvReflectCreateShaderModule(shader.byteCode->GetBufferSize(), shader.byteCode->GetBufferPointer(), &reflectionModule) != SPV_REFLECT_RESULT_SUCCESS)
			return VK_NULL_HANDLE;

		if (RemapDescriptorSets(reflectionModule) == false) {
			spvReflectDestroyShaderModule(&reflectionModule);
			return VK_NULL_HANDLE;
		}

		VkShaderModuleCreateInfo shaderModuleCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = spvReflectGetCodeSize(&reflectionModule),
			.pCode = spvReflectGetCode(&reflectionModule),
		};

		VkShaderModule module = VK_NULL_HANDLE;

		if (vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &module) == VK_SUCCESS) {
			if (shader.shaderType == VK_SHADER_STAGE_VERTEX_BIT)
				SPIRVVariableReflection::FillInputAssemblyReflection(m_inputAssemblyReflection, &reflectionModule);
			else if (shader.shaderType == VK_SHADER_STAGE_FRAGMENT_BIT)
				SPIRVVariableReflection::FillRenderTargetReflection(m_renderTargetReflection, &reflectionModule);

			SPIRVVariableReflection::AddShaderReflection(m_variableReflection, &reflectionModule);
		}

		spvReflectDestroyShaderModule(&reflectionModule);
		return module;
	}

	bool SPIRVRasterizationProgram::InitializePipelineLayout(std::string& error)
	{
		if (m_vertexShader == VK_NULL_HANDLE) {
			error = "Vertex stage is missing or failed to create";
			return false;
		}

		InitializeSampler();

		return SPIRVVariableReflection::CreatePipelineLayout(m_device, m_variableReflection, EntitySetIndex + 1, StageFlags, m_sampler,
			m_descriptorSetLayouts, m_pipelineLayout, error);
	}
}
