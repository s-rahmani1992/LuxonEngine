#include "vulkan-pch.h"
#include "SPIRVRayTracingProgramVariant.h"
#include "Core/VulkanDeviceManager.h"

QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgramVariant::SPIRVRayTracingProgramVariant(Byte* byteCode, UInt32 codesize)
	:m_byteCode(byteCode), m_codeSize(codesize),
	m_device(VulkanDeviceManager::Instance()->GetGraphicDevice())
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = m_codeSize,
		.pCode = reinterpret_cast<UInt32*>(m_byteCode),
	};
	vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &m_rtModule);

	spvReflectCreateShaderModule(m_codeSize, m_byteCode, &m_reflectionModule);
	
	auto shaderStageFlags = 0;
}

QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgramVariant::~SPIRVRayTracingProgramVariant()
{
	vkDestroyShaderModule(m_device, m_rtModule, nullptr);
	delete[] m_byteCode;
}

std::vector<VkPipelineShaderStageCreateInfo> QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgramVariant::GetStages()
{
	SpvReflectEntryPoint* rayGenEntryPoint = nullptr;
	SpvReflectEntryPoint* missEntryPoint = nullptr;
	SpvReflectEntryPoint* closestHitEntryPoint = nullptr;
	SpvReflectEntryPoint* anyHitEntryPoint = nullptr;
	SpvReflectEntryPoint* intersectionEntryPoint = nullptr;

	for (UInt32 i = 0; i < m_reflectionModule.entry_point_count; i++) {
		auto& entryPoint = m_reflectionModule.entry_points[i];

		switch (entryPoint.spirv_execution_model) {
		case SpvExecutionModelRayGenerationKHR:
			rayGenEntryPoint = &entryPoint;
			//shaderStageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			break;
		case SpvExecutionModelMissKHR:
			missEntryPoint = &entryPoint;
			//shaderStageFlags |= VK_SHADER_STAGE_MISS_BIT_KHR;
			break;
		case SpvExecutionModelClosestHitKHR:
			closestHitEntryPoint = &entryPoint;
			//shaderStageFlags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			break;
		case SpvExecutionModelAnyHitKHR:
			anyHitEntryPoint = &entryPoint;
			//shaderStageFlags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			break;
		case SpvExecutionModelIntersectionKHR:
			intersectionEntryPoint = &entryPoint;
			//shaderStageFlags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			break;
		}
	}

	std::vector<VkPipelineShaderStageCreateInfo> stages;
	if (rayGenEntryPoint) {
		VkPipelineShaderStageCreateInfo rayGenStage{};
		rayGenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rayGenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		rayGenStage.module = m_rtModule;
		rayGenStage.pName = rayGenEntryPoint->name;
		stages.push_back(rayGenStage);
	}
	if (missEntryPoint) {
		VkPipelineShaderStageCreateInfo missStage{};
		missStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		missStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		missStage.module = m_rtModule;
		missStage.pName = missEntryPoint->name;
		stages.push_back(missStage);
	}
	if (closestHitEntryPoint) {
		VkPipelineShaderStageCreateInfo closestHitStage{};
		closestHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		closestHitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		closestHitStage.module = m_rtModule;
		closestHitStage.pName = closestHitEntryPoint->name;
		stages.push_back(closestHitStage);
	}
	if (anyHitEntryPoint) {
		VkPipelineShaderStageCreateInfo anyHitStage{};
		anyHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		anyHitStage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		anyHitStage.module = m_rtModule;
		anyHitStage.pName = anyHitEntryPoint->name;
		stages.push_back(anyHitStage);
	}
	if (intersectionEntryPoint) {
		VkPipelineShaderStageCreateInfo intersectionStage{};
		intersectionStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		intersectionStage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		intersectionStage.module = m_rtModule;
		intersectionStage.pName = intersectionEntryPoint->name;
		stages.push_back(intersectionStage);
	}
	return stages;
}

void QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgramVariant::GetBindingAndSet(const std::string& name, UInt32* binding, UInt32* set)
{
	uint32_t binding_count = 0;
	spvReflectEnumerateDescriptorBindings(&m_reflectionModule, &binding_count, nullptr);

	std::vector<SpvReflectDescriptorBinding*> descriptorBindings(binding_count);
	spvReflectEnumerateDescriptorBindings(&m_reflectionModule, &binding_count, descriptorBindings.data());

	auto it = std::find_if(descriptorBindings.begin(), descriptorBindings.end(), [&name](const SpvReflectDescriptorBinding* descriptor) {
		return name == descriptor->name;
		});

	*binding = (*it)->binding;
	*set = (*it)->set;
}
