#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	class SPIRVMeshProgram;

	class VulkanMeshPipelineModule {
	public:
		VulkanMeshPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVMeshProgram* program, Material* material);

		~VulkanMeshPipelineModule();

		VulkanMeshPipelineModule(const VulkanMeshPipelineModule&) = delete;
		VulkanMeshPipelineModule& operator=(const VulkanMeshPipelineModule&) = delete;

		inline VkPipeline GetPipeline() const { return m_pipeline; }
		inline const SPIRVMeshProgram* GetProgram() const { return m_meshProgram; }
		inline Material* GetMaterial() const { return m_material; }

	private:
		VkDevice m_device;
		VkPipeline m_pipeline;
		const SPIRVMeshProgram* m_meshProgram;
		Material* m_material;
	};
}
