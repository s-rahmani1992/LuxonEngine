#include "vulkan-pch.h"
#include "VulkanComputePipelineModule.h"
#include "SPIRVComputeProgram.h"

namespace LuxonEngine::Rendering::Vulkan::Compute {
	VulkanComputePipelineModule::VulkanComputePipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVComputeProgram* program, Material* material)
		:m_device(device), m_pipeline(pipeline), m_program(program), m_material(material)
	{
		m_resourceManager = std::make_unique<VulkanMaterialResourceManager>(device, material, program->GetVariableReflection(),
			program->GetDescriptorSetLayouts(), program->GetPipelineLayout(), SPIRVComputeProgram::StageFlags, VK_PIPELINE_BIND_POINT_COMPUTE);
	}

	VulkanComputePipelineModule::~VulkanComputePipelineModule()
	{
		if (m_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}

	bool VulkanComputePipelineModule::Initialize(UInt32 entityCount)
	{
		if (m_resourceManager->Initialize(entityCount) == false)
			return false;

		m_threadGroupCounts.resize(entityCount);
		return true;
	}

	void VulkanComputePipelineModule::UpdateModifiedTextures()
	{
		m_resourceManager->UpdateModifiedTextures();
	}

	bool VulkanComputePipelineModule::SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		return m_resourceManager->SetDescriptor(name, buffer, offset, range);
	}

	bool VulkanComputePipelineModule::SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		return m_resourceManager->SetDescriptor(name, imageView, imageLayout);
	}

	bool VulkanComputePipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, buffer, offset, range);
	}

	bool VulkanComputePipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, imageView, imageLayout);
	}

	bool VulkanComputePipelineModule::SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y, UInt32 z)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == VulkanMaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_threadGroupCounts.size() <= slot)
			m_threadGroupCounts.resize(slot + 1);

		m_threadGroupCounts[slot] = { x, y, z };
		return true;
	}

	void VulkanComputePipelineModule::BindGlobal(VkCommandBuffer commandBuffer)
	{
		m_resourceManager->UpdateModifiedTextures();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

		m_resourceManager->BindGlobal(commandBuffer);
	}

	bool VulkanComputePipelineModule::DispatchSlot(VkCommandBuffer commandBuffer, UInt32 slot)
	{
		if (slot >= m_threadGroupCounts.size() || m_resourceManager->GetEntity(slot) == nullptr)
			return false;

		auto& groupCount = m_threadGroupCounts[slot];

		if (groupCount[0] == 0 || groupCount[1] == 0 || groupCount[2] == 0)
			return false;

		m_resourceManager->BindEntity(commandBuffer, slot);
		vkCmdDispatch(commandBuffer, groupCount[0], groupCount[1], groupCount[2]);
		return true;
	}

	bool VulkanComputePipelineModule::DispatchEntity(VkCommandBuffer commandBuffer, GameEntity* entity)
	{
		UInt32 slot = m_resourceManager->FindEntity(entity);

		if (slot == VulkanMaterialResourceManager::InvalidEntityIndex)
			return false;

		return DispatchSlot(commandBuffer, slot);
	}

	void VulkanComputePipelineModule::Dispatch(VkCommandBuffer commandBuffer)
	{
		BindGlobal(commandBuffer);

		for (UInt32 slot = 0; slot < m_resourceManager->GetEntitySlotCount(); slot++)
			DispatchSlot(commandBuffer, slot);
	}
}
