#include "vulkan-pch.h"
#include "VulkanRasterizationPipelineModule.h"
#include "SPIRVRasterizationProgram.h"

namespace LuxonEngine::Rendering::Vulkan::Rasterization {
	VulkanRasterizationPipelineModule::VulkanRasterizationPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVRasterizationProgram* program, Material* material)
		:m_device(device), m_pipeline(pipeline), m_program(program), m_material(material)
	{
		m_resourceManager = std::make_unique<VulkanMaterialResourceManager>(device, material, program->GetVariableReflection(),
			program->GetDescriptorSetLayouts(), program->GetPipelineLayout(), SPIRVRasterizationProgram::StageFlags, VK_PIPELINE_BIND_POINT_GRAPHICS);
	}

	VulkanRasterizationPipelineModule::~VulkanRasterizationPipelineModule()
	{
		if (m_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}

	bool VulkanRasterizationPipelineModule::Initialize(UInt32 entityCount)
	{
		if (m_resourceManager->Initialize(entityCount) == false)
			return false;

		m_entityGeometries.resize(entityCount);
		return true;
	}

	void VulkanRasterizationPipelineModule::UpdateModifiedTextures()
	{
		m_resourceManager->UpdateModifiedTextures();
	}

	bool VulkanRasterizationPipelineModule::SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		return m_resourceManager->SetDescriptor(name, buffer, offset, range);
	}

	bool VulkanRasterizationPipelineModule::SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		return m_resourceManager->SetDescriptor(name, imageView, imageLayout);
	}

	bool VulkanRasterizationPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, buffer, offset, range);
	}

	bool VulkanRasterizationPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, imageView, imageLayout);
	}

	bool VulkanRasterizationPipelineModule::SetEntityGeometry(GameEntity* entity, VkBuffer vertexBuffer, VkBuffer indexBuffer, UInt32 indexCount)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == VulkanMaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_entityGeometries.size() <= slot)
			m_entityGeometries.resize(slot + 1);

		m_entityGeometries[slot] = EntityGeometry{ vertexBuffer, indexBuffer, indexCount, 0 };
		return true;
	}

	bool VulkanRasterizationPipelineModule::SetEntityGeometry(GameEntity* entity, VkBuffer vertexBuffer, UInt32 vertexCount)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == VulkanMaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_entityGeometries.size() <= slot)
			m_entityGeometries.resize(slot + 1);

		m_entityGeometries[slot] = EntityGeometry{ vertexBuffer, VK_NULL_HANDLE, 0, vertexCount };
		return true;
	}

	void VulkanRasterizationPipelineModule::BindGlobal(VkCommandBuffer commandBuffer)
	{
		m_resourceManager->UpdateModifiedTextures();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		m_resourceManager->BindGlobal(commandBuffer);
	}

	bool VulkanRasterizationPipelineModule::DrawSlot(VkCommandBuffer commandBuffer, UInt32 slot)
	{
		if (slot >= m_entityGeometries.size() || m_resourceManager->GetEntity(slot) == nullptr)
			return false;

		auto& geometry = m_entityGeometries[slot];

		if (geometry.vertexBuffer == VK_NULL_HANDLE || (geometry.indexCount == 0 && geometry.vertexCount == 0))
			return false;

		m_resourceManager->BindEntity(commandBuffer, slot);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &geometry.vertexBuffer, &offset);

		if (geometry.indexCount > 0) {
			vkCmdBindIndexBuffer(commandBuffer, geometry.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, geometry.indexCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(commandBuffer, geometry.vertexCount, 1, 0, 0);
		}

		return true;
	}

	bool VulkanRasterizationPipelineModule::DrawEntity(VkCommandBuffer commandBuffer, GameEntity* entity)
	{
		UInt32 slot = m_resourceManager->FindEntity(entity);

		if (slot == VulkanMaterialResourceManager::InvalidEntityIndex)
			return false;

		return DrawSlot(commandBuffer, slot);
	}

	void VulkanRasterizationPipelineModule::Draw(VkCommandBuffer commandBuffer)
	{
		BindGlobal(commandBuffer);

		for (UInt32 slot = 0; slot < m_resourceManager->GetEntitySlotCount(); slot++)
			DrawSlot(commandBuffer, slot);
	}
}
