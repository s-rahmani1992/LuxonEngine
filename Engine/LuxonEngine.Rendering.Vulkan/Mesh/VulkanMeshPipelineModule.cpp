#include "vulkan-pch.h"
#include "VulkanMeshPipelineModule.h"
#include "SPIRVMeshProgram.h"
#include "Rendering/Material.h"

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	VulkanMeshPipelineModule::VulkanMeshPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVMeshProgram* program, Material* material)
		:m_device(device), m_pipeline(pipeline), m_meshProgram(program), m_material(material)
	{
	}

	VulkanMeshPipelineModule::~VulkanMeshPipelineModule()
	{
		// the pipeline layout belongs to the program, only the pipeline is owned by the module
		if (m_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}
}
