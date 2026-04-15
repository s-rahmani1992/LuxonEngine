#include "vulkan-pch.h"
#include "SPIRVRayTracingProgram.h"
#include "SPIRVRayTracingProgramVariant.h"
#include "Core/VulkanUtilities.h"

QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::SPIRVRayTracingProgram(Byte* bytecode, UInt64 codeSize, VkDevice device)
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

	vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &m_rtModule);
	
	spvReflectCreateShaderModule(m_codeSize, m_bytecode, &m_reflectionModule);
	m_reflection.AddShaderReflection(&m_reflectionModule);

	m_reflection.Initializes();

	UInt32 inputCount;

	spvReflectEnumerateInterfaceVariables(&m_reflectionModule, &inputCount, nullptr);
	std::vector<SpvReflectInterfaceVariable*> inputs(inputCount);
	spvReflectEnumerateInterfaceVariables(&m_reflectionModule, &inputCount, inputs.data());
	
	auto shaderRecordIT = std::find_if(inputs.begin(), inputs.end(), [](SpvReflectInterfaceVariable* interfaceVar) {
		return interfaceVar->storage_class == SpvStorageClassShaderRecordBufferKHR;
		});

	m_shaderRecord = shaderRecordIT != inputs.end() ? *shaderRecordIT : nullptr;
	
	if (m_shaderRecord != nullptr) {
		UInt32 varOffset = 0;
		auto typeDescription = m_shaderRecord->members[0].type_description;
		for (UInt32 i = 0; i < typeDescription->member_count; i++) {
			auto member = typeDescription->members[i];
			auto numeric = member.traits.numeric;
			auto varSize = (numeric.scalar.width / 8) * (numeric.vector.component_count == 0 ? 1 : numeric.vector.component_count);
			m_shaderRecordVariables.push_back(ShaderRecordVariableData
				{
					.name = std::string(member.struct_member_name),
					.offset = varOffset,
					.size = varSize,
				});

			varOffset += varSize;
		}

	}
	
	auto shaderStageFlags = 0;

	for(UInt32 i = 0; i < m_reflectionModule.entry_point_count; i++) {
		auto& entryPoint = m_reflectionModule.entry_points[i];
		
		switch(entryPoint.spirv_execution_model) {
			case SpvExecutionModelRayGenerationKHR:
				m_rayGenEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				break;
			case SpvExecutionModelMissKHR:
				m_missEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_MISS_BIT_KHR;
				break;
			case SpvExecutionModelClosestHitKHR:
				m_closestHitEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				break;
			case SpvExecutionModelAnyHitKHR:
				m_anyHitEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				break;
			case SpvExecutionModelIntersectionKHR:
				m_intersectionEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				break;
		}
	}
}

QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::~SPIRVRayTracingProgram()
{
	vkDestroyShaderModule(m_device, m_rtModule, nullptr);

	delete[] m_bytecode;
}

std::vector<VkPipelineShaderStageCreateInfo> QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::GetShaderStages()
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	if(m_rayGenEntryPoint) {
		VkPipelineShaderStageCreateInfo rayGenStage{};
		rayGenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rayGenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		rayGenStage.module = m_rtModule;
		rayGenStage.pName = m_rayGenEntryPoint->name;
		stages.push_back(rayGenStage);
	}
	if(m_missEntryPoint) {
		VkPipelineShaderStageCreateInfo missStage{};
		missStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		missStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		missStage.module = m_rtModule;
		missStage.pName = m_missEntryPoint->name;
		stages.push_back(missStage);
	}
	if(m_closestHitEntryPoint) {
		VkPipelineShaderStageCreateInfo closestHitStage{};
		closestHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		closestHitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		closestHitStage.module = m_rtModule;
		closestHitStage.pName = m_closestHitEntryPoint->name;
		stages.push_back(closestHitStage);
	}
	if(m_anyHitEntryPoint) {
		VkPipelineShaderStageCreateInfo anyHitStage{};
		anyHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		anyHitStage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		anyHitStage.module = m_rtModule;
		anyHitStage.pName = m_anyHitEntryPoint->name;
		stages.push_back(anyHitStage);
	}
	if(m_intersectionEntryPoint) {
		VkPipelineShaderStageCreateInfo intersectionStage{};
		intersectionStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		intersectionStage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		intersectionStage.module = m_rtModule;
		intersectionStage.pName = m_intersectionEntryPoint->name;
		stages.push_back(intersectionStage);
	}
	return stages;
}

ref<QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgramVariant> QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::CreateVariantForRT(UInt32& startBinding)
{
	Byte* variantByteCode = new Byte[m_codeSize];
	std::memcpy(variantByteCode, m_bytecode, m_codeSize);

	UInt32 descriptorCount;

	std::map<std::string, UInt32> specialNames = {
		{HLSL_CAMERA_DATA_NAME, 1},
		{HLSL_LIGHT_DATA_NAME, 2},
		{HLSL_TRANSFORM_ARRAY, 3},
		{HLSL_RT_TLAS_SCENE_NAME, 4},
		{HLSL_RT_OUTPUT_TEXTURE_NAME, 5},
		{HLSL_RT_VERTEX_BUFFER_ARRAY, 6},
		{HLSL_RT_INDEX_BUFFER_ARRAY, 7},
	};

	spvReflectEnumerateDescriptorBindings(&m_reflectionModule, &descriptorCount, nullptr);
	std::vector<SpvReflectDescriptorBinding*> descriptors(descriptorCount);
	spvReflectEnumerateDescriptorBindings(&m_reflectionModule, &descriptorCount, descriptors.data());

	UInt32* wordByteCode = (UInt32*)variantByteCode;

	for (auto& descriptor : descriptors) {

		if (descriptor->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER) {
			*(wordByteCode + descriptor->word_offset.set) = 1;
			*(wordByteCode + descriptor->word_offset.binding) = 0;
			continue;
		}

		UInt32 binding = *(wordByteCode + descriptor->word_offset.binding);
		
		auto nameIt = specialNames.find(descriptor->name);

		if (nameIt != specialNames.end()) {
			*(wordByteCode + descriptor->word_offset.set) = 1;
			*(wordByteCode + descriptor->word_offset.binding) = (*nameIt).second;
			continue;
		}

		*(wordByteCode + descriptor->word_offset.set) = 0;
		*(wordByteCode + descriptor->word_offset.binding) = startBinding;
		startBinding++;
	}

	return std::make_shared<SPIRVRayTracingProgramVariant>(variantByteCode, m_codeSize);
}

UInt32 QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::GetShaderRecordSize()
{
	if (m_shaderRecord == nullptr)
		return 0;

	UInt32 size = 0;

	for (auto& var : m_shaderRecordVariables)
		size += var.size;
	
	return size;
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::HasHitGroup()
{
	return m_closestHitEntryPoint != nullptr ||
		m_anyHitEntryPoint != nullptr ||
		m_intersectionEntryPoint != nullptr;
}
