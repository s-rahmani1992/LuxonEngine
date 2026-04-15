#include "vulkan-pch.h"
#include "VulkanRasterizationMaterial.h"
#include "Rendering/Material.h"
#include "SPIRVRasterizationProgram.h"
#include "Core/Texture2D.h"
#include "Core/VulkanTexture2DController.h"

QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::VulkanRasterizationMaterial(const ref<Material>& material, const VkDevice device)
	:m_material(material), m_device(device), m_program(std::dynamic_pointer_cast<SPIRVRasterizationProgram>(material->GetProgram()))
{
	m_pipelineLayout = m_program->GetPipelineLayout();
	auto& reflection = m_program->GetReflection();
	auto& pushConstants = reflection.GetPushConstants();

	for(auto& pushConstantBlock : pushConstants.blocks)
	{
		auto valueLocation = material->GetValueLocation(pushConstantBlock.variables[0].name);
		if (valueLocation != nullptr)
		{
			m_pushConstantValues.push_back({
				.offset = pushConstantBlock.offset,
				.location = valueLocation,
				.size = pushConstantBlock.size,
				});
		}
	}

	auto& descriptors = reflection.GetDescriptors();
	auto textureFields = material->GetTextureFields();
	UInt32 fieldIndex = 0;

	for (auto& descriptor : descriptors) {
		if (descriptor.name[0] == '_')
			continue;

		m_descriptorData.push_back(DescriptorData{
			.setIndex = descriptor.data.set,
			.binding = descriptor.data.binding,
			.descriptorType = descriptor.descriptorType,
			});

		auto matIt = textureFields->find(descriptor.name);

		if (matIt != textureFields->end())
			(*matIt).second.fieldIndex = fieldIndex;

		fieldIndex++;
	}
}

bool QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::Initialize(const VkDescriptorPool pool)
{
	auto& layouts = m_program->GetDiscriptorLayouts();
	m_descriptorSets.resize(layouts.size());
	m_descriptorData.resize(m_material->GetTextureFields()->size());

	VkDescriptorSetAllocateInfo descSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = nullptr,
	};

	for (UInt32 i = 0; i < layouts.size(); i++) {
		descSetAlloc.pSetLayouts = layouts.data() + i;

		if (vkAllocateDescriptorSets(m_device, &descSetAlloc, m_descriptorSets.data() + i) != VK_SUCCESS)
			return false;
	}
	
	return true;
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::BindValues(VkCommandBuffer commandBuffer)
{
	// Update Modified Textures
	for (auto& modified : m_material->GetModifiedTextures()) {
		VkDescriptorImageInfo imageInfo{
			.sampler = nullptr,
			.imageView = std::dynamic_pointer_cast<VulkanTexture2DController>(modified->texture->GetGPUHandle())->GetImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptorSets[m_descriptorData[modified->fieldIndex].setIndex],
		.dstBinding = m_descriptorData[modified->fieldIndex].binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = m_descriptorData[modified->fieldIndex].descriptorType,
		.pImageInfo = &imageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
		};

		vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
	}

	m_material->ClearModifiedTextures();

	for(auto& pushConstant : m_pushConstantValues)
	{
		vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, pushConstant.offset, pushConstant.size, pushConstant.location);
	}
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::BindDynamicValues(VkCommandBuffer commandBuffer, UInt32* offsets, UInt32 offsetCount)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, (UInt32)m_descriptorSets.size(), m_descriptorSets.data(), offsetCount, offsets);
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::WriteBuffer(const std::string name, const VkBuffer buffer, UInt32 stride)
{
	auto descriptorData = m_program->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	VkDescriptorBufferInfo descBufferInfo{
		.buffer = buffer,
		.offset = 0,
		.range = stride,
	};

	VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptorSets[descriptorData->data.set],
		.dstBinding = descriptorData->data.binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorData->descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = &descBufferInfo,
		.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::SetImageView(const std::string& name, const VkImageView imageView)
{
	auto descriptorData = m_program->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	VkDescriptorImageInfo imageInfo{
			.sampler = nullptr,
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkWriteDescriptorSet writeDescriptor{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.pNext = nullptr,
	.dstSet = m_descriptorSets[descriptorData->data.set],
	.dstBinding = descriptorData->data.binding,
	.dstArrayElement = 0,
	.descriptorCount = 1,
	.descriptorType = descriptorData->descriptorType,
	.pImageInfo = &imageInfo,
	.pBufferInfo = nullptr,
	.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
}
