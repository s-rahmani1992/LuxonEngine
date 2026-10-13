#include "vulkan-pch.h"
#include "VulkanMaterialResourceManager.h"
#include "SPIRVShaderProgram.h"
#include "SPIRVReflection.h"
#include "Rendering/Material.h"
#include "Rendering/ShaderInternalNames.h"
#include "Core/Texture2D.h"
#include "Core/VulkanTexture2DController.h"
#include <algorithm>

namespace LuxonEngine::Rendering::Vulkan {
	VulkanMaterialResourceManager::VulkanMaterialResourceManager(VkDevice device, Material* material, const ShaderVariableReflection& reflection,
		const std::vector<VkDescriptorSetLayout>& setLayouts, VkPipelineLayout pipelineLayout, VkShaderStageFlags stageFlags, VkPipelineBindPoint bindPoint)
		:m_device(device), m_material(material), m_reflection(reflection), m_setLayouts(setLayouts), m_pipelineLayout(pipelineLayout), m_stageFlags(stageFlags), m_bindPoint(bindPoint)
	{
	}

	VulkanMaterialResourceManager::~VulkanMaterialResourceManager()
	{
		for (auto& entityData : m_entityDataList) {
			if (entityData.descriptorPool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_device, entityData.descriptorPool, nullptr);

			for (auto& constant : entityData.constants)
				delete[] constant.data;
		}

		if (m_materialDescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_device, m_materialDescriptorPool, nullptr);
	}

	bool VulkanMaterialResourceManager::Initialize(UInt32 entityCount)
	{
		InitializeDescriptorMapping();
		InitializeConstantMapping();

		if (CreateDescriptorSet(m_materialPoolSizes, SPIRVShaderProgram::GlobalSetIndex, m_materialDescriptorPool, m_materialDescriptorSet) == false)
			return false;

		for (UInt32 i = 0; i < entityCount; i++) {
			EntityDynamicData entityData;

			if (CreateEntityData(nullptr, entityData) == false)
				return false;

			m_entityDataList.push_back(entityData);
		}

		CopyMaterialTextures();
		return true;
	}

	void VulkanMaterialResourceManager::InitializeDescriptorMapping()
	{
		for (auto& resourceVariable : m_reflection.resources) {
			DescriptorData descriptorData{
				.name = resourceVariable.name.c_str(),
				.binding = resourceVariable.binding,
				.descriptorType = SPIRVVariableReflection::ToDescriptorType(resourceVariable.kind),
				.descriptorCount = resourceVariable.count,
			};

			if (resourceVariable.space == SPIRVShaderProgram::EntitySetIndex) {
				m_entityDescriptorLayout[resourceVariable.name] = (UInt32)m_entityDescriptors.size();
				m_entityDescriptors.push_back(descriptorData);
				m_entityPoolSizes.push_back(VkDescriptorPoolSize{ .type = descriptorData.descriptorType, .descriptorCount = descriptorData.descriptorCount });
				continue;
			}

			m_materialDescriptors.push_back(descriptorData);
			m_materialPoolSizes.push_back(VkDescriptorPoolSize{ .type = descriptorData.descriptorType, .descriptorCount = descriptorData.descriptorCount });
		}

		for (auto& samplerVariable : m_reflection.samplers) {
			auto& poolSizes = samplerVariable.space == SPIRVShaderProgram::EntitySetIndex ? m_entityPoolSizes : m_materialPoolSizes;
			poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1 });
		}

		if (m_material == nullptr)
			return;

		auto textureFields = m_material->GetTextureFields();
		m_textureDescriptorIndices = std::vector<UInt32>(textureFields->size(), ShaderVariableReflection::InvalidIndex);

		for (UInt32 i = 0; i < m_materialDescriptors.size(); i++) {
			auto it = textureFields->find(m_materialDescriptors[i].name);

			if (it != textureFields->end())
				m_textureDescriptorIndices[it->second.fieldIndex] = i;
		}
	}

	bool VulkanMaterialResourceManager::CreateDescriptorSet(const std::vector<VkDescriptorPoolSize>& poolSizes, UInt32 setIndex, VkDescriptorPool& pool, VkDescriptorSet& set) const
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

		VkDescriptorSetLayout setLayout = m_setLayouts[setIndex];

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

	void VulkanMaterialResourceManager::InitializeConstantMapping()
	{
		for (auto& block : m_reflection.constants.blocks) {
			if (block.variables.empty())
				continue;

			ConstantData constantData{
				.name = block.variables[0].name.c_str(),
				.offset = block.offset,
				.size = block.size,
			};

			if (block.isInternal == false) {
				constantData.data = m_material == nullptr ? nullptr : m_material->GetValueLocation(block.variables[0].name);
				m_pushConstants.push_back(constantData);
				continue;
			}

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

	bool VulkanMaterialResourceManager::CreateEntityData(GameEntity* entity, EntityDynamicData& entityData) const
	{
		entityData.entity = entity;

		if (CreateDescriptorSet(m_entityPoolSizes, SPIRVShaderProgram::EntitySetIndex, entityData.descriptorPool, entityData.descriptorSet) == false)
			return false;

		entityData.constants = m_entityConstantTemplate;

		for (auto& constant : entityData.constants)
			constant.data = new Byte[constant.size]();

		return true;
	}

	VulkanMaterialResourceManager::EntityDynamicData* VulkanMaterialResourceManager::GetOrRegisterEntity(GameEntity* entity)
	{
		auto entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [entity](const EntityDynamicData& entityData) {
			return entityData.entity == entity;
			});

		if (entityIT != m_entityDataList.end())
			return &(*entityIT);

		entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [](const EntityDynamicData& entityData) {
			return entityData.entity == nullptr;
			});

		if (entityIT != m_entityDataList.end()) {
			entityIT->entity = entity;
			return &(*entityIT);
		}

		EntityDynamicData entityData;

		if (CreateEntityData(entity, entityData) == false)
			return nullptr;

		m_entityDataList.push_back(entityData);
		return &m_entityDataList.back();
	}

	UInt32 VulkanMaterialResourceManager::RegisterEntity(GameEntity* entity)
	{
		if (entity == nullptr)
			return InvalidEntityIndex;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return InvalidEntityIndex;

		return (UInt32)(entityData - m_entityDataList.data());
	}

	UInt32 VulkanMaterialResourceManager::FindEntity(GameEntity* entity) const
	{
		auto entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [entity](const EntityDynamicData& entityData) {
			return entityData.entity == entity;
			});

		if (entity == nullptr || entityIT == m_entityDataList.end())
			return InvalidEntityIndex;

		return (UInt32)(entityIT - m_entityDataList.begin());
	}

	bool VulkanMaterialResourceManager::SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size)
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

	void VulkanMaterialResourceManager::UpdateModifiedTextures()
	{
		if (m_material == nullptr || m_materialDescriptorSet == VK_NULL_HANDLE)
			return;

		for (auto* textureData : m_material->GetModifiedTextures())
			WriteTextureDescriptor(*textureData);

		m_material->ClearTextures();
	}

	void VulkanMaterialResourceManager::BindGlobal(VkCommandBuffer commandBuffer)
	{
		if (m_materialDescriptorSet != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(commandBuffer, m_bindPoint, m_pipelineLayout,
				SPIRVShaderProgram::GlobalSetIndex, 1, &m_materialDescriptorSet, 0, nullptr);
		}

		for (auto& constant : m_pushConstants) {
			if (constant.data != nullptr)
				vkCmdPushConstants(commandBuffer, m_pipelineLayout, m_stageFlags, constant.offset, constant.size, constant.data);
		}
	}

	void VulkanMaterialResourceManager::BindEntity(VkCommandBuffer commandBuffer, UInt32 slot)
	{
		auto& entityData = m_entityDataList[slot];

		if (entityData.descriptorSet != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(commandBuffer, m_bindPoint, m_pipelineLayout,
				SPIRVShaderProgram::EntitySetIndex, 1, &entityData.descriptorSet, 0, nullptr);
		}

		for (auto& constant : entityData.constants)
			vkCmdPushConstants(commandBuffer, m_pipelineLayout, m_stageFlags, constant.offset, constant.size, constant.data);
	}

	void VulkanMaterialResourceManager::CopyMaterialTextures()
	{
		if (m_material == nullptr)
			return;

		for (auto& [name, textureData] : *m_material->GetTextureFields()) {
			if (textureData.texture != nullptr)
				WriteTextureDescriptor(textureData);
		}
	}

	void VulkanMaterialResourceManager::WriteTextureDescriptor(const MaterialTextureData& textureData)
	{
		if (m_materialDescriptorSet == VK_NULL_HANDLE || textureData.fieldIndex >= m_textureDescriptorIndices.size())
			return;

		UInt32 descriptorIndex = m_textureDescriptorIndices[textureData.fieldIndex];

		if (descriptorIndex == ShaderVariableReflection::InvalidIndex || textureData.texture == nullptr)
			return;

		auto vulkanTexture = std::dynamic_pointer_cast<VulkanTexture2DController>(textureData.texture->GetGPUHandle());

		if (vulkanTexture == nullptr)
			return;

		VkDescriptorImageInfo imageInfo{
			.sampler = VK_NULL_HANDLE,
			.imageView = vulkanTexture->GetImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		WriteDescriptor(m_materialDescriptorSet, m_materialDescriptors[descriptorIndex], nullptr, &imageInfo);
	}

	bool VulkanMaterialResourceManager::SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
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

	bool VulkanMaterialResourceManager::SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout)
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

	bool VulkanMaterialResourceManager::SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
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

	bool VulkanMaterialResourceManager::SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout)
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

	const VulkanMaterialResourceManager::DescriptorData* VulkanMaterialResourceManager::FindGlobalDescriptor(const char* name) const
	{
		if (name == nullptr || ShaderInternalNames::IsGlobal(name) == false)
			return nullptr;

		auto it = std::find_if(m_materialDescriptors.begin(), m_materialDescriptors.end(), [name](const DescriptorData& descriptorData) {
			return strcmp(descriptorData.name, name) == 0;
			});

		if (it == m_materialDescriptors.end())
			return nullptr;

		return &(*it);
	}

	const VulkanMaterialResourceManager::DescriptorData* VulkanMaterialResourceManager::FindEntityDescriptor(const char* name) const
	{
		if (name == nullptr || ShaderInternalNames::IsPerEntity(name) == false)
			return nullptr;

		auto layoutIT = m_entityDescriptorLayout.find(name);

		if (layoutIT == m_entityDescriptorLayout.end())
			return nullptr;

		return &m_entityDescriptors[layoutIT->second];
	}

	bool VulkanMaterialResourceManager::WriteDescriptor(VkDescriptorSet set, const DescriptorData& descriptorData, const VkDescriptorBufferInfo* bufferInfo, const VkDescriptorImageInfo* imageInfo)
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
		default:
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
