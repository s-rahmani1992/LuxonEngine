#include "vulkan-pch.h"
#include "VulkanMeshPipelineModule.h"
#include "SPIRVMeshProgram.h"

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	VulkanMeshPipelineModule::VulkanMeshPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVMeshProgram* program, Material* material)
		:m_device(device), m_pipeline(pipeline), m_meshProgram(program), m_material(material)
	{
		m_cmdDrawMeshTasks = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(m_device, "vkCmdDrawMeshTasksEXT");

		m_resourceManager = std::make_unique<VulkanMaterialResourceManager>(device, material, program->GetVariableReflection(),
			program->GetDescriptorSetLayouts(), program->GetPipelineLayout(), SPIRVMeshProgram::StageFlags, VK_PIPELINE_BIND_POINT_GRAPHICS);
	}

	VulkanMeshPipelineModule::~VulkanMeshPipelineModule()
	{
		if (m_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}

	bool VulkanMeshPipelineModule::Initialize(UInt32 entityCount)
	{
		if (m_resourceManager->Initialize(entityCount) == false)
			return false;

		m_entityThreadGroupCounts.resize(entityCount);
		return true;
	}

	void VulkanMeshPipelineModule::UpdateModifiedTextures()
	{
		m_resourceManager->UpdateModifiedTextures();
	}

	void VulkanMeshPipelineModule::Dispatch(VkCommandBuffer commandBuffer)
	{
		if (m_pipeline == VK_NULL_HANDLE || m_cmdDrawMeshTasks == nullptr)
			return;

		m_resourceManager->UpdateModifiedTextures();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		m_resourceManager->BindGlobal(commandBuffer);

		for (UInt32 slot = 0; slot < m_resourceManager->GetEntitySlotCount() && slot < m_entityThreadGroupCounts.size(); slot++) {
			auto& groupCount = m_entityThreadGroupCounts[slot];

			if (m_resourceManager->GetEntity(slot) == nullptr || groupCount.x == 0 || groupCount.y == 0 || groupCount.z == 0)
				continue;

			m_resourceManager->BindEntity(commandBuffer, slot);
			m_cmdDrawMeshTasks(commandBuffer, groupCount.x, groupCount.y, groupCount.z);
		}
	}

	bool VulkanMeshPipelineModule::SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y, UInt32 z)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == VulkanMaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_entityThreadGroupCounts.size() <= slot)
			m_entityThreadGroupCounts.resize(slot + 1);

		m_entityThreadGroupCounts[slot] = ThreadGroupCount{ x, y, z };
		return true;
	}

	bool VulkanMeshPipelineModule::SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		return m_resourceManager->SetDescriptor(name, buffer, offset, range);
	}

	bool VulkanMeshPipelineModule::SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		return m_resourceManager->SetDescriptor(name, imageView, imageLayout);
	}

	bool VulkanMeshPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, buffer, offset, range);
	}

	bool VulkanMeshPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, imageView, imageLayout);
	}
}
