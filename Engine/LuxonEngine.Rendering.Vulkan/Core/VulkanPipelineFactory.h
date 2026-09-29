#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"
#include <string>

namespace LuxonEngine::Rendering {
	class Material;
	class ShaderProgram;
}

namespace LuxonEngine::Rendering::Vulkan {
	namespace MeshShading {
		class VulkanMeshPipelineModule;
		class SPIRVMeshProgram;

		struct MeshPipelineProperties {
			bool enableDepthTest = true;
		};
	}

	class VulkanPipelineFactory {
	public:
		VulkanPipelineFactory(VkDevice device);

		ref<MeshShading::VulkanMeshPipelineModule> CreateMeshPipeline(Material* material, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error);
		ref<MeshShading::VulkanMeshPipelineModule> CreateMeshPipeline(const ShaderProgram* program, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error);

	private:
		ref<MeshShading::VulkanMeshPipelineModule> CreateMeshPipeline(const ShaderProgram* program, Material* material, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error);

	private:
		VkDevice m_device;
	};
}
