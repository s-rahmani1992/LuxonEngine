#include "vulkan-pch.h"
#include "VulkanMeshPipelineModule.h"
#include "SPIRVMeshProgram.h"
#include "Rendering/Material.h"
#include "Rendering/ShaderInternalNames.h"
#include "Core/Texture2D.h"
#include "Core/VulkanTexture2DController.h"
#include <algorithm>

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	VulkanMeshPipelineModule::VulkanMeshPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVMeshProgram* program, Material* material)
		:m_device(device), m_pipeline(pipeline), m_meshProgram(program), m_material(material)
	{
		m_cmdDrawMeshTasks = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(m_device, "vkCmdDrawMeshTasksEXT");
	}

	VulkanMeshPipelineModule::~VulkanMeshPipelineModule()
	{
		// destroying a pool frees the sets allocated from it.
		// the data of the dynamic constant blocks is owned by the entity data, the material blocks point to the material
		for (auto& entityData : m_entityDataList) {
			if (entityData.descriptorPool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_device, entityData.descriptorPool, nullptr);

			for (auto& constant : entityData.constants)
				delete[] constant.data;
		}

		if (m_materialDescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_device, m_materialDescriptorPool, nullptr);

		// the pipeline layout belongs to the program, only the pipeline is owned by the module
		if (m_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}

	bool VulkanMeshPipelineModule::Initialize(UInt32 entityCount)
	{
		InitializeDescriptorMapping();
		InitializeConstantMapping();

		if (CreateDescriptorSet(m_materialPoolSizes, SPIRVMeshProgram::GlobalSetIndex, m_materialDescriptorPool, m_materialDescriptorSet) == false)
			return false;

		// the reserved entities are registered later, their sets are written when they are registered
		for (UInt32 i = 0; i < entityCount; i++) {
			EntityDynamicData entityData;

			if (CreateEntityData(nullptr, entityData) == false)
				return false;

			m_entityDataList.push_back(entityData);
		}

		CopyMaterialTextures();
		return true;
	}

	void VulkanMeshPipelineModule::InitializeDescriptorMapping()
	{
		auto& variableReflection = m_meshProgram->GetVariableReflection();

		// the set of a variable is already remapped by the program, set 1 is per entity and set 0 is the material and global variables
		for (auto& resourceVariable : variableReflection.resources) {
			DescriptorData descriptorData{
				.name = resourceVariable.name.c_str(),
				.binding = resourceVariable.binding,
				.descriptorType = SPIRVVariableReflection::ToDescriptorType(resourceVariable.kind),
				.descriptorCount = resourceVariable.count,
			};

			if (resourceVariable.space == SPIRVMeshProgram::EntitySetIndex) {
				m_entityDescriptorLayout[resourceVariable.name] = (UInt32)m_entityDescriptors.size();
				m_entityDescriptors.push_back(descriptorData);
				m_entityPoolSizes.push_back(VkDescriptorPoolSize{ .type = descriptorData.descriptorType, .descriptorCount = descriptorData.descriptorCount });
				continue;
			}

			m_materialDescriptors.push_back(descriptorData);
			m_materialPoolSizes.push_back(VkDescriptorPoolSize{ .type = descriptorData.descriptorType, .descriptorCount = descriptorData.descriptorCount });
		}

		// samplers are immutable samplers of the layout, they are never written but they still take space in the pool
		for (auto& samplerVariable : variableReflection.samplers) {
			auto& poolSizes = samplerVariable.space == SPIRVMeshProgram::EntitySetIndex ? m_entityPoolSizes : m_materialPoolSizes;
			poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1 });
		}

		if (m_material == nullptr)
			return;

		// lookup table from the texture field index of the material to the descriptor of this pipeline
		auto textureFields = m_material->GetTextureFields();
		m_textureDescriptorIndices = std::vector<UInt32>(textureFields->size(), ShaderVariableReflection::InvalidIndex);

		for (UInt32 i = 0; i < m_materialDescriptors.size(); i++) {
			auto it = textureFields->find(m_materialDescriptors[i].name);

			if (it != textureFields->end())
				m_textureDescriptorIndices[it->second.fieldIndex] = i;
		}
	}

	bool VulkanMeshPipelineModule::CreateDescriptorSet(const std::vector<VkDescriptorPoolSize>& poolSizes, UInt32 setIndex, VkDescriptorPool& pool, VkDescriptorSet& set) const
	{
		if (poolSizes.empty())
			return true;

		VkDescriptorPoolCreateInfo poolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = 1,
			.poolSizeCount = (UInt32)poolSizes.size(),
			.pPoolSizes = poolSizes.data(),
		};

		if (vkCreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &pool) != VK_SUCCESS)
			return false;

		VkDescriptorSetLayout setLayout = m_meshProgram->GetDescriptorSetLayouts()[setIndex];

		VkDescriptorSetAllocateInfo setAllocateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &setLayout,
		};

		if (vkAllocateDescriptorSets(m_device, &setAllocateInfo, &set) != VK_SUCCESS) {
			vkDestroyDescriptorPool(m_device, pool, nullptr);
			pool = VK_NULL_HANDLE;
			return false;
		}

		return true;
	}

	void VulkanMeshPipelineModule::InitializeConstantMapping()
	{
		auto& constants = m_meshProgram->GetVariableReflection().constants;

		for (auto& block : constants.blocks) {
			if (block.variables.empty())
				continue;

			ConstantData constantData{
				.name = block.variables[0].name.c_str(),
				.offset = block.offset,
				.size = block.size,
			};

			// material block: the variables are consecutive in the material data, starting from the first one
			if (block.isInternal == false) {
				constantData.data = m_material == nullptr ? nullptr : m_material->GetValueLocation(block.variables[0].name);
				m_pushConstants.push_back(constantData);
				continue;
			}

			// dynamic block: the data is allocated per entity when the entity data is created
			UInt32 blockIndex = (UInt32)m_entityConstantTemplate.size();

			for (auto& variable : block.variables) {
				m_entityConstantLayout[variable.name] = ConstantVariableLocation{
					.blockIndex = blockIndex,
					.offset = variable.offset - block.offset,
					.size = variable.size,
				};
			}

			m_entityConstantTemplate.push_back(constantData);
		}
	}

	bool VulkanMeshPipelineModule::CreateEntityData(GameEntity* entity, EntityDynamicData& entityData) const
	{
		entityData.entity = entity;

		if (CreateDescriptorSet(m_entityPoolSizes, SPIRVMeshProgram::EntitySetIndex, entityData.descriptorPool, entityData.descriptorSet) == false)
			return false;

		entityData.constants = m_entityConstantTemplate;

		for (auto& constant : entityData.constants)
			constant.data = new Byte[constant.size]();

		return true;
	}

	bool VulkanMeshPipelineModule::SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size)
	{
		if (entity == nullptr || name == nullptr)
			return false;

		auto layoutIT = m_entityConstantLayout.find(name);

		if (layoutIT == m_entityConstantLayout.end() || layoutIT->second.size != size)
			return false;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return false;

		auto& location = layoutIT->second;
		memcpy(entityData->constants[location.blockIndex].data + location.offset, data, size);
		return true;
	}

	VulkanMeshPipelineModule::EntityDynamicData* VulkanMeshPipelineModule::GetOrRegisterEntity(GameEntity* entity)
	{
		auto entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [entity](const EntityDynamicData& entityData) {
			return entityData.entity == entity;
			});

		if (entityIT != m_entityDataList.end())
			return &(*entityIT);

		// take a free slot reserved by Initialize
		entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [](const EntityDynamicData& entityData) {
			return entityData.entity == nullptr;
			});

		if (entityIT != m_entityDataList.end()) {
			entityIT->entity = entity;
			return &(*entityIT);
		}

		// every entity has its own pool, so the existing sets stay valid when a new entity is added
		EntityDynamicData entityData;

		if (CreateEntityData(entity, entityData) == false)
			return nullptr;

		m_entityDataList.push_back(entityData);
		return &m_entityDataList.back();
	}

	void VulkanMeshPipelineModule::UpdateModifiedTextures()
	{
		if (m_material == nullptr || m_materialDescriptorSet == VK_NULL_HANDLE)
			return;

		for (auto* textureData : m_material->GetModifiedTextures())
			WriteTextureDescriptor(*textureData);

		// only the textures are cleared, the modified values are read directly from the material data
		m_material->ClearTextures();
	}

	void VulkanMeshPipelineModule::Dispatch(VkCommandBuffer commandBuffer)
	{
		if (m_pipeline == VK_NULL_HANDLE || m_cmdDrawMeshTasks == nullptr)
			return;

		UpdateModifiedTextures();

		VkPipelineLayout pipelineLayout = m_meshProgram->GetPipelineLayout();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		// global and material bindings are shared by all the entities. binding the entity set later keeps this set bound
		if (m_materialDescriptorSet != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				SPIRVMeshProgram::GlobalSetIndex, 1, &m_materialDescriptorSet, 0, nullptr);
		}

		for (auto& constant : m_pushConstants) {
			if (constant.data != nullptr)
				vkCmdPushConstants(commandBuffer, pipelineLayout, SPIRVMeshProgram::StageFlags, constant.offset, constant.size, constant.data);
		}

		// every registered entity is dispatched with its own group count
		for (auto& entityData : m_entityDataList) {
			auto& groupCount = entityData.threadGroupCount;

			if (entityData.entity == nullptr || groupCount[0] == 0 || groupCount[1] == 0 || groupCount[2] == 0)
				continue;

			if (entityData.descriptorSet != VK_NULL_HANDLE) {
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
					SPIRVMeshProgram::EntitySetIndex, 1, &entityData.descriptorSet, 0, nullptr);
			}

			for (auto& constant : entityData.constants)
				vkCmdPushConstants(commandBuffer, pipelineLayout, SPIRVMeshProgram::StageFlags, constant.offset, constant.size, constant.data);

			m_cmdDrawMeshTasks(commandBuffer, groupCount[0], groupCount[1], groupCount[2]);
		}
	}

	bool VulkanMeshPipelineModule::SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y, UInt32 z)
	{
		if (entity == nullptr)
			return false;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return false;

		entityData->threadGroupCount[0] = x;
		entityData->threadGroupCount[1] = y;
		entityData->threadGroupCount[2] = z;
		return true;
	}

	void VulkanMeshPipelineModule::CopyMaterialTextures()
	{
		if (m_material == nullptr)
			return;

		for (auto& [name, textureData] : *m_material->GetTextureFields()) {
			if (textureData.texture != nullptr)
				WriteTextureDescriptor(textureData);
		}
	}

	void VulkanMeshPipelineModule::WriteTextureDescriptor(const MaterialTextureData& textureData)
	{
		if (m_materialDescriptorSet == VK_NULL_HANDLE || textureData.fieldIndex >= m_textureDescriptorIndices.size())
			return;

		UInt32 descriptorIndex = m_textureDescriptorIndices[textureData.fieldIndex];

		if (descriptorIndex == ShaderVariableReflection::InvalidIndex || textureData.texture == nullptr)
			return;

		auto vulkanTexture = std::dynamic_pointer_cast<VulkanTexture2DController>(textureData.texture->GetGPUHandle());

		if (vulkanTexture == nullptr) // the texture is not uploaded to the GPU yet
			return;

		VkDescriptorImageInfo imageInfo{
			.sampler = VK_NULL_HANDLE,
			.imageView = vulkanTexture->GetImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		WriteDescriptor(m_materialDescriptorSet, m_materialDescriptors[descriptorIndex], nullptr, &imageInfo);
	}

	bool VulkanMeshPipelineModule::SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		const DescriptorData* descriptorData = FindGlobalDescriptor(name);

		if (descriptorData == nullptr)
			return false;

		VkDescriptorBufferInfo bufferInfo{
			.buffer = buffer,
			.offset = offset,
			.range = range,
		};

		return WriteDescriptor(m_materialDescriptorSet, *descriptorData, &bufferInfo, nullptr);
	}

	bool VulkanMeshPipelineModule::SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		const DescriptorData* descriptorData = FindGlobalDescriptor(name);

		if (descriptorData == nullptr)
			return false;

		VkDescriptorImageInfo imageInfo{
			.sampler = VK_NULL_HANDLE,
			.imageView = imageView,
			.imageLayout = imageLayout,
		};

		return WriteDescriptor(m_materialDescriptorSet, *descriptorData, nullptr, &imageInfo);
	}

	bool VulkanMeshPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		const DescriptorData* descriptorData = FindEntityDescriptor(name);

		if (entity == nullptr || descriptorData == nullptr)
			return false;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return false;

		VkDescriptorBufferInfo bufferInfo{
			.buffer = buffer,
			.offset = offset,
			.range = range,
		};

		return WriteDescriptor(entityData->descriptorSet, *descriptorData, &bufferInfo, nullptr);
	}

	bool VulkanMeshPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		const DescriptorData* descriptorData = FindEntityDescriptor(name);

		if (entity == nullptr || descriptorData == nullptr)
			return false;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return false;

		VkDescriptorImageInfo imageInfo{
			.sampler = VK_NULL_HANDLE,
			.imageView = imageView,
			.imageLayout = imageLayout,
		};

		return WriteDescriptor(entityData->descriptorSet, *descriptorData, nullptr, &imageInfo);
	}

	const VulkanMeshPipelineModule::DescriptorData* VulkanMeshPipelineModule::FindGlobalDescriptor(const char* name) const
	{
		if (name == nullptr || ShaderInternalNames::IsGlobal(name) == false)
			return nullptr;

		auto it = std::find_if(m_materialDescriptors.begin(), m_materialDescriptors.end(), [name](const DescriptorData& descriptorData) {
			return strcmp(descriptorData.name, name) == 0;
			});

		if (it == m_materialDescriptors.end()) // the program does not use this global variable
			return nullptr;

		return &(*it);
	}

	const VulkanMeshPipelineModule::DescriptorData* VulkanMeshPipelineModule::FindEntityDescriptor(const char* name) const
	{
		if (name == nullptr || ShaderInternalNames::IsPerEntity(name) == false)
			return nullptr;

		auto layoutIT = m_entityDescriptorLayout.find(name);

		if (layoutIT == m_entityDescriptorLayout.end()) // the program does not use this per entity variable
			return nullptr;

		return &m_entityDescriptors[layoutIT->second];
	}

	bool VulkanMeshPipelineModule::WriteDescriptor(VkDescriptorSet set, const DescriptorData& descriptorData, const VkDescriptorBufferInfo* bufferInfo, const VkDescriptorImageInfo* imageInfo)
	{
		if (set == VK_NULL_HANDLE)
			return false;

		switch (descriptorData.descriptorType) {
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			if (bufferInfo == nullptr)
				return false;
			break;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			if (imageInfo == nullptr)
				return false;
			break;
		default: // samplers are immutable and acceleration structures are written differently
			return false;
		}

		VkWriteDescriptorSet writeDescriptor{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = set,
			.dstBinding = descriptorData.binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = descriptorData.descriptorType,
			.pImageInfo = imageInfo,
			.pBufferInfo = bufferInfo,
			.pTexelBufferView = nullptr,
		};

		vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
		return true;
	}
}
