#include "vulkan-pch.h"
#include "SPIRVReflection.h"
#include <algorithm>

void LuxonEngine::Rendering::Vulkan::SPIRVReflection::AddShaderReflection(const SpvReflectShaderModule* shaderReflectionModule, bool isRayTracing)
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

UInt32 LuxonEngine::Rendering::Vulkan::SPIRVReflection::GetDescriptorLayoutCount()
{
	auto h = std::max_element(m_descripters.begin(), m_descripters.end()
		, [](const DescriptableBufferData& desc1, const DescriptableBufferData& desc2) {
			return desc1.data.set < desc2.data.set;
		});

	return (*h).data.set + 1;
}

UInt32 LuxonEngine::Rendering::Vulkan::SPIRVReflection::GetDynamicDescriptorCount()
{
	auto h = std::count_if(m_descripters.begin(), m_descripters.end(), [](const DescriptableBufferData& desc) {
		return (desc.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC || desc.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		});
	return UInt32(h);
}

void LuxonEngine::Rendering::Vulkan::SPIRVReflection::CreatePipelineLayout(const VkDevice device, VkShaderStageFlags stageFlags, const VkSampler sampler, VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout* descriptorSetLayout)
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

LuxonEngine::Rendering::MaterialReflection LuxonEngine::Rendering::Vulkan::SPIRVReflection::CreateMaterialReflection()
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

LuxonEngine::Rendering::Vulkan::PushConstantBlockData* LuxonEngine::Rendering::Vulkan::SPIRVReflection::GetPushConstantBlockData(const std::string& name)
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

LuxonEngine::Rendering::Vulkan::DescriptableBufferData* LuxonEngine::Rendering::Vulkan::SPIRVReflection::GetDescriptorData(const std::string name)
{
	auto it = std::find_if(m_descripters.begin(), m_descripters.end(), [name](const DescriptableBufferData& desc) {
		return desc.name == name;
		});

	if (it == m_descripters.end())
		return nullptr;

	return &(*it);
}

void LuxonEngine::Rendering::Vulkan::SPIRVReflection::Initializes()
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

LuxonEngine::Rendering::ShaderStageFlags LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::ToShaderStage(SpvReflectShaderStageFlagBits stage)
{
	switch (stage) {
	case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:			return ShaderStageFlags::Vertex;
	case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:			return ShaderStageFlags::Geometry;
	case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:			return ShaderStageFlags::Pixel;
	case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:			return ShaderStageFlags::Compute;
	case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:		return ShaderStageFlags::RayGeneration;
	case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:			return ShaderStageFlags::Miss;
	case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:	return ShaderStageFlags::ClosestHit;
	case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:		return ShaderStageFlags::AnyHit;
	case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:	return ShaderStageFlags::Intersection;
	case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:		return ShaderStageFlags::Callable;
	case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT:			return ShaderStageFlags::Amplification;
	case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT:			return ShaderStageFlags::Mesh;
	default:											return ShaderStageFlags::None;
	}
}

std::string LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::ShaderStageToString(SpvReflectShaderStageFlagBits stage)
{
	switch (stage) {
	case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:			return "Vertex";
	case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:			return "Geometry";
	case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:			return "Pixel";
	case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:			return "Compute";
	case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:		return "RayGeneration";
	case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:			return "Miss";
	case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:	return "ClosestHit";
	case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:		return "AnyHit";
	case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:	return "Intersection";
	case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:		return "Callable";
	case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT:			return "Amplification";
	case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT:			return "Mesh";
	default:											return "Unknown";
	}
}

LuxonEngine::Rendering::ShaderResourceKind LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::ToResourceKind(const SpvReflectDescriptorBinding* descriptor)
{
	// HLSL read only and read write buffers map to the same descriptor type, the resource type keeps the HLSL register class
	bool isReadWrite = (descriptor->resource_type & SPV_REFLECT_RESOURCE_FLAG_UAV) != 0;

	switch (descriptor->descriptor_type) {
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
		return ShaderResourceKind::Sampler;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return ShaderResourceKind::ConstantBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return ShaderResourceKind::Texture;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return ShaderResourceKind::RWTexture;
	case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
		return ShaderResourceKind::AccelerationStructure;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return ShaderResourceKind::StructuredBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return ShaderResourceKind::RWStructuredBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return isReadWrite ? ShaderResourceKind::RWStructuredBuffer : ShaderResourceKind::StructuredBuffer;
	default:
		return ShaderResourceKind::StructuredBuffer;
	}
}

LuxonEngine::Rendering::ShaderScalarType LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::ToScalarType(SpvReflectTypeFlags typeFlags, const SpvReflectNumericTraits& numericTraits)
{
	if ((typeFlags & SPV_REFLECT_TYPE_FLAG_BOOL) != 0)
		return ShaderScalarType::Bool;

	if ((typeFlags & SPV_REFLECT_TYPE_FLAG_INT) != 0)
		return numericTraits.scalar.signedness == 0 ? ShaderScalarType::UInt : ShaderScalarType::Int;

	if ((typeFlags & SPV_REFLECT_TYPE_FLAG_FLOAT) != 0) {
		switch (numericTraits.scalar.width) {
		case 16: return ShaderScalarType::Half;
		case 64: return ShaderScalarType::Double;
		default: return ShaderScalarType::Float;
		}
	}

	return ShaderScalarType::Unknown;
}

void LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::AddConstants(ShaderVariableReflection& reflection, const SpvReflectBlockVariable* pushConstant, ShaderStageFlags stage)
{
	auto& constants = reflection.constants;

	constants.name = pushConstant->name == nullptr ? "" : pushConstant->name;
	constants.size = pushConstant->size;
	constants.stages |= stage;

	UInt32 index = 0;

	while (index < pushConstant->member_count) {
		bool internalBlock = ShaderVariableReflection::IsInternalName(pushConstant->members[index].name);

		auto& blockItem = constants.blocks.emplace_back(ShaderConstantBlock{
			.offset = pushConstant->members[index].offset,
			.size = 0,
			.isInternal = internalBlock,
			});

		while (index < pushConstant->member_count && internalBlock == ShaderVariableReflection::IsInternalName(pushConstant->members[index].name)) {
			auto& member = pushConstant->members[index];
			SpvReflectTypeFlags typeFlags = member.type_description == nullptr ? 0 : member.type_description->type_flags;

			UInt32 rows = 1;
			UInt32 columns = 1;

			if ((typeFlags & SPV_REFLECT_TYPE_FLAG_MATRIX) != 0) {
				rows = member.numeric.matrix.row_count;
				columns = member.numeric.matrix.column_count;
			}
			else if ((typeFlags & SPV_REFLECT_TYPE_FLAG_VECTOR) != 0) {
				columns = member.numeric.vector.component_count;
			}

			blockItem.variables.push_back(ShaderConstantVariable{
				.name = member.name,
				.index = index,
				.offset = member.offset,
				.size = member.size,
				.type = ToScalarType(typeFlags, member.numeric),
				.rows = rows,
				.columns = columns,
				});
			blockItem.size += member.size;

			index++;
		}
	}
}

void LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::AddShaderReflection(ShaderVariableReflection& reflection, const SpvReflectShaderModule* shaderReflectionModule)
{
	ShaderStageFlags stage = ToShaderStage(shaderReflectionModule->shader_stage);

	// Extract the push constants
	UInt32 pushConstantCount = 0;
	spvReflectEnumeratePushConstantBlocks(shaderReflectionModule, &pushConstantCount, nullptr);

	std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
	spvReflectEnumeratePushConstantBlocks(shaderReflectionModule, &pushConstantCount, pushConstants.data());

	for (auto& pushConstant : pushConstants) {
		if (reflection.constants.blocks.empty() == false) {
			reflection.constants.stages |= stage;
			continue; //the constant data can exist in multiple shaders of the program
		}

		AddConstants(reflection, pushConstant, stage);
	}

	// Extract the descriptors
	UInt32 bindingCount = 0;
	spvReflectEnumerateDescriptorBindings(shaderReflectionModule, &bindingCount, nullptr);

	std::vector<SpvReflectDescriptorBinding*> descriptorBindings(bindingCount);
	spvReflectEnumerateDescriptorBindings(shaderReflectionModule, &bindingCount, descriptorBindings.data());

	for (auto& descriptor : descriptorBindings) {
		std::string name(descriptor->name == nullptr ? "" : descriptor->name);
		ShaderResourceKind kind = ToResourceKind(descriptor);

		auto& list = kind == ShaderResourceKind::Sampler ? reflection.samplers : reflection.resources;

		auto it = std::find_if(list.begin(), list.end(), [descriptor](const ShaderResourceVariable& item) {
			return item.binding == descriptor->binding && item.space == descriptor->set;
			});

		if (it != list.end()) {
			it->stages |= stage; //the resource can exist in multiple shaders of the program
			continue;
		}

		list.push_back(ShaderResourceVariable{
			.name = name,
			.kind = kind,
			.binding = descriptor->binding,
			.space = descriptor->set,
			.count = descriptor->count,
			.isUnbounded = (descriptor->array.dims_count > 0) && (descriptor->array.dims[0] == 0), // runtime arrays are reported with a size of 0
			.isInternal = ShaderVariableReflection::IsInternalName(name),
			.stages = stage,
			});
	}
}

void LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::FillThreadGroupReflection(ShaderThreadGroupReflection& reflection, const SpvReflectShaderModule* shaderReflectionModule)
{
	if (shaderReflectionModule->entry_point_count == 0)
		return;

	auto& localSize = shaderReflectionModule->entry_points[0].local_size;

	reflection.xThread = localSize.x;
	reflection.yThread = localSize.y;
	reflection.zThread = localSize.z;
}

VkDescriptorType LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::ToDescriptorType(ShaderResourceKind kind)
{
	switch (kind) {
	case ShaderResourceKind::ConstantBuffer:		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case ShaderResourceKind::StructuredBuffer:
	case ShaderResourceKind::RWStructuredBuffer:	return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case ShaderResourceKind::Texture:				return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case ShaderResourceKind::RWTexture:				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case ShaderResourceKind::Sampler:				return VK_DESCRIPTOR_TYPE_SAMPLER;
	case ShaderResourceKind::AccelerationStructure:	return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	default:										return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
}

bool LuxonEngine::Rendering::Vulkan::SPIRVVariableReflection::CreatePipelineLayout(VkDevice device, const ShaderVariableReflection& reflection, UInt32 setCount,
	VkShaderStageFlags stageFlags, VkSampler sampler, std::vector<VkDescriptorSetLayout>& setLayouts, VkPipelineLayout& pipelineLayout, std::string& error)
{
	std::vector<std::vector<VkDescriptorSetLayoutBinding>> setBindings(setCount);

	auto addBinding = [&setBindings, &error, stageFlags, &sampler](const ShaderResourceVariable& variable) {
		if (variable.space >= setBindings.size()) {
			error = "Invalid descriptor set " + std::to_string(variable.space) + " for variable " + variable.name;
			return false;
		}

		if (variable.isUnbounded) {
			error = "Unbounded arrays are not supported: " + variable.name;
			return false;
		}

		bool isSampler = variable.kind == ShaderResourceKind::Sampler;

		// the immutable sampler is copied into the layout when it is created, so pointing to the parameter is enough
		setBindings[variable.space].push_back(VkDescriptorSetLayoutBinding{
			.binding = variable.binding,
			.descriptorType = ToDescriptorType(variable.kind),
			.descriptorCount = isSampler ? 1 : variable.count,
			.stageFlags = stageFlags,
			.pImmutableSamplers = isSampler ? &sampler : nullptr,
			});

		return true;
	};

	for (auto& resource : reflection.resources) {
		if (addBinding(resource) == false)
			return false;
	}

	for (auto& samplerVariable : reflection.samplers) {
		if (addBinding(samplerVariable) == false)
			return false;
	}

	auto destroySetLayouts = [device, &setLayouts]() {
		for (auto& setLayout : setLayouts) {
			if (setLayout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
		}
		setLayouts.clear();
	};

	setLayouts.assign(setBindings.size(), VK_NULL_HANDLE);

	for (UInt32 i = 0; i < setBindings.size(); i++) {
		VkDescriptorSetLayoutCreateInfo setLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = (UInt32)setBindings[i].size(),
			.pBindings = setBindings[i].empty() ? nullptr : setBindings[i].data(),
		};

		if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &setLayouts[i]) != VK_SUCCESS) {
			error = "Failed to create descriptor set layout " + std::to_string(i);
			destroySetLayouts();
			return false;
		}
	}

	// all the constant blocks belong to one push constant block, material and dynamic ones
	VkPushConstantRange pushConstantRange{
		.stageFlags = stageFlags,
		.offset = 0,
		.size = reflection.constants.size,
	};

	bool hasPushConstants = reflection.constants.blocks.empty() == false;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = (UInt32)setLayouts.size(),
		.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data(),
		.pushConstantRangeCount = hasPushConstants ? 1u : 0u,
		.pPushConstantRanges = hasPushConstants ? &pushConstantRange : nullptr,
	};

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		error = "Failed to create pipeline layout";
		destroySetLayouts();
		return false;
	}

	return true;
}
