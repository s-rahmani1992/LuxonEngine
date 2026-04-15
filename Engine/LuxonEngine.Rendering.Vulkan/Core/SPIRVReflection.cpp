#include "vulkan-pch.h"
#include "SPIRVReflection.h"
#include <algorithm>

void QuantumEngine::Rendering::Vulkan::SPIRVReflection::AddShaderReflection(const SpvReflectShaderModule* shaderReflectionModule, bool isRayTracing)
{
	UInt32 pc_count = 0;
	spvReflectEnumeratePushConstantBlocks(shaderReflectionModule, &pc_count, nullptr);
	std::vector<SpvReflectBlockVariable*> pushConstants(pc_count);
	spvReflectEnumeratePushConstantBlocks(shaderReflectionModule, &pc_count, pushConstants.data());

	for(auto& pushConstant : pushConstants)
	{
		if(m_pushConstant.blocks.size() > 0)//checking if the push constant is processed because it can exist in multiple shaders
			continue;

		m_pushConstant.name = pushConstant->name;
		m_pushConstant.data = *pushConstant;
		
		UInt32 i = 0;

		do {
			if (pushConstant->members[i].name[0] == '_') {
				auto& blockItem = m_pushConstant.blocks.emplace_back(PushConstantBlockData{
				.offset = pushConstant->members[i].offset,
				.size = 0,
				.isDynamic = true,
					});
				UInt32 size = 0;

				while (pushConstant->members[i].name[0] == '_') {
					blockItem.variables.push_back(PushConstantVariableData{
						.name = pushConstant->members[i].name,
						.variableDesc = pushConstant->members[i],
						});
					blockItem.size += pushConstant->members[i].size;

					i++;

					if (i >= pushConstant->member_count)
						break;
				}
			}

			else {
				auto& blockItem = m_pushConstant.blocks.emplace_back(PushConstantBlockData{
				.offset = pushConstant->members[i].offset,
				.size = 0,
				.isDynamic = false,
					});

				while (pushConstant->members[i].name[0] != '_') {
					blockItem.variables.push_back(PushConstantVariableData{
						.name = pushConstant->members[i].name,
						.variableDesc = pushConstant->members[i],
						});

					blockItem.size += pushConstant->members[i].size;
					i++;

					if (i >= pushConstant->member_count)
						break;
				}
			}
		} while (i < pushConstant->member_count);
	}

	// Extract Descriptors
	uint32_t binding_count = 0;
	spvReflectEnumerateDescriptorBindings(shaderReflectionModule, &binding_count, nullptr);
	
	std::vector<SpvReflectDescriptorBinding*> descriptorBindings(binding_count);
	spvReflectEnumerateDescriptorBindings(shaderReflectionModule, &binding_count, descriptorBindings.data());
	
	for (auto& descriptor : descriptorBindings) {
		auto it = std::find_if(m_descripters.begin(), m_descripters.end(),
			[descriptor](const DescriptableBufferData& data) { return data.data.binding == descriptor->binding && data.data.set == descriptor->set; });

		if (it != m_descripters.end())//checking if the resource is already processed because it can exist in multiple shaders
			continue;

		VkDescriptorType desType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		if (descriptor->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER) {
			m_samplers.push_back(DescriptableBufferData{
				.name = std::string(descriptor->name),
				.offsetIndex = UINT32_MAX,
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
				.data = *descriptor,
			});
			continue;
		}

		switch (descriptor->descriptor_type) {
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			desType = (isRayTracing || descriptor->name[0] != '_') ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			desType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			desType = (isRayTracing || descriptor->name[0] != '_') ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			desType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			desType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			break;
		}

		bool isDynamic = (descriptor->array.dims_count > 0) &&
			(descriptor->array.dims[0] == 0);

		m_descripters.push_back(DescriptableBufferData{
			.name = std::string(descriptor->name),
			.isDynamicArray = isDynamic,
			.offsetIndex = UINT32_MAX,
			.descriptorType = desType,
			.data = *descriptor,
		});
	}

}

UInt32 QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetDescriptorLayoutCount()
{
	auto h = std::max_element(m_descripters.begin(), m_descripters.end()
		, [](const DescriptableBufferData& desc1, const DescriptableBufferData& desc2) {
			return desc1.data.set < desc2.data.set;
		});

	return (*h).data.set + 1;
}

UInt32 QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetDynamicDescriptorCount()
{
	auto h = std::count_if(m_descripters.begin(), m_descripters.end(), [](const DescriptableBufferData& desc) {
		return (desc.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC || desc.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		});
	return UInt32(h);
}

void QuantumEngine::Rendering::Vulkan::SPIRVReflection::CreatePipelineLayout(const VkDevice device, VkShaderStageFlags stageFlags, const VkSampler sampler, VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout* descriptorSetLayout)
{
	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.reserve(1);

	pushConstantRanges.push_back(VkPushConstantRange{
			.stageFlags = stageFlags,
			.offset = m_pushConstant.data.offset,
			.size = m_pushConstant.data.size,
		});

	VkDescriptorType desType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	std::vector<std::vector<VkDescriptorSetLayoutBinding>> descriptorLayoutBindings(GetDescriptorLayoutCount());
	std::vector<std::vector<VkDescriptorBindingFlags>> bindingFlags(GetDescriptorLayoutCount());

	for (auto& descriptor : m_descripters) {
		bindingFlags[descriptor.data.set].push_back(descriptor.isDynamicArray ? VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);

		descriptorLayoutBindings[descriptor.data.set].push_back(VkDescriptorSetLayoutBinding{
			.binding = descriptor.data.binding,
			.descriptorType = descriptor.descriptorType,
			.descriptorCount = descriptor.isDynamicArray ? 200 : descriptor.data.count, //TODO fix te hard-coded size
			.stageFlags = stageFlags,
			.pImmutableSamplers = nullptr,
		});
	}

	std::set<std::pair<UInt32, UInt32>> samplerBindings;

	for (auto& samplerDescriptor : m_samplers) {
		auto it = samplerBindings.emplace(samplerDescriptor.data.set, samplerDescriptor.data.binding);

		if(it.second == false)//checking if the sampler binding is already processed because it can exist in multiple shaders
			continue;

		descriptorLayoutBindings[samplerDescriptor.data.set].push_back(VkDescriptorSetLayoutBinding{
			.binding = samplerDescriptor.data.binding,
			.descriptorType = samplerDescriptor.descriptorType,
			.descriptorCount = samplerDescriptor.data.count,
			.stageFlags = stageFlags,
			.pImmutableSamplers = &sampler,
			});
	}

	for (UInt32 i = 0; i < descriptorLayoutBindings.size(); i++) {
		VkDescriptorSetLayoutBindingFlagsCreateInfo createInfoFlag{};
		createInfoFlag.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		createInfoFlag.bindingCount = (UInt32)bindingFlags[i].size();         
		createInfoFlag.pBindingFlags = bindingFlags[i].data();

		VkDescriptorSetLayoutCreateInfo descriptorCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = (UInt32)descriptorLayoutBindings[i].size(),
		.pBindings = descriptorLayoutBindings[i].data(),
		};

		vkCreateDescriptorSetLayout(device, &descriptorCreateInfo, nullptr, descriptorSetLayout + i);
	}

	UInt32 pcCount = m_pushConstant.blocks.size() == 0 ? 0 : 1;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = (UInt32)descriptorLayoutBindings.size(),
		.pSetLayouts = descriptorSetLayout,
		.pushConstantRangeCount = pcCount,
		.pPushConstantRanges = m_pushConstant.blocks.size() == 0 ? nullptr : pushConstantRanges.data(),
	};

	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout);
}

QuantumEngine::Rendering::MaterialReflection QuantumEngine::Rendering::Vulkan::SPIRVReflection::CreateMaterialReflection()
{
	UInt32 fieldIndex = 0;
	MaterialReflection reflectionData;

	for (auto& pushConstBlock : m_pushConstant.blocks) {
		if (pushConstBlock.isDynamic) {// Skip internal root constants
			fieldIndex++;
			continue;
		}

		for (auto& rootVar : pushConstBlock.variables) {
			MaterialValueFieldInfo valueFieldInfo{
				.name = rootVar.name,
				.fieldIndex = fieldIndex,
				.size = rootVar.variableDesc.size,
			};
			reflectionData.valueFields.push_back(valueFieldInfo);
			fieldIndex++;
		}
	}

	UInt32 textureIndex = 0;
	for (auto& descriptor : m_descripters) {
		if (descriptor.name[0] == '_') {// Skip internal variables
			textureIndex++;
			continue;
		}

		MaterialTextureFieldInfo textureFieldInfo{
			.name = descriptor.isDynamicArray ? descriptor.name.substr(0, descriptor.name.size() - 5) : descriptor.name,
			.fieldIndex = textureIndex,
		};

		reflectionData.textureFields.push_back(textureFieldInfo);
		textureIndex++;
	}

	return reflectionData;
}

QuantumEngine::Rendering::Vulkan::PushConstantBlockData* QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetPushConstantBlockData(const std::string& name)
{
	for (auto& block : m_pushConstant.blocks) {
		auto varIt = std::find_if(block.variables.begin(), block.variables.end(), [name](const PushConstantVariableData& var) {
			return var.name == name;
			});

		if (varIt != block.variables.end())
			return &block;
	}
	return nullptr;
}

QuantumEngine::Rendering::Vulkan::DescriptableBufferData* QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetDescriptorData(const std::string name)
{
	auto it = std::find_if(m_descripters.begin(), m_descripters.end(), [name](const DescriptableBufferData& desc) {
		return desc.name == name;
		});

	if (it == m_descripters.end())
		return nullptr;

	return &(*it);
}

void QuantumEngine::Rendering::Vulkan::SPIRVReflection::Initializes()
{
	if (m_descripters.size() == 0)
		return;

	auto h = std::max_element(m_descripters.begin(), m_descripters.end()
		, [](const DescriptableBufferData& desc1, const DescriptableBufferData& desc2) {
			return desc1.data.set < desc2.data.set;
		});

	std::vector<UInt32> offsets((*h).data.set + 1, 0);
	std::vector<UInt32> finalOffsets((*h).data.set + 1, 0);

	UInt32 offsetIndex = UINT32_MAX;

	for (auto& descriptor : m_descripters) {
		if (descriptor.name[0] != '_' || (descriptor.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC && descriptor.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
			continue;

		descriptor.offsetIndex = offsets[descriptor.data.set];
		offsets[descriptor.data.set]++;
	}

	for (int i = 1; i < finalOffsets.size(); i++)
		finalOffsets[i] = finalOffsets[i - 1] + offsets[i - 1];

	for (auto& descriptor : m_descripters) {

		if (descriptor.name[0] != '_' || (descriptor.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC && descriptor.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
			continue;

		descriptor.offsetIndex += finalOffsets[descriptor.data.set];
	}
}
