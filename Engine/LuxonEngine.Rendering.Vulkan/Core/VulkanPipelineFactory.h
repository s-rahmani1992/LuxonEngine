#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"
#include <string>
#include <vector>

namespace LuxonEngine {
	class GameEntity;
}

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

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
		class SPIRVRasterizationProgram;

		struct RasterizationPipelineProperties {
			bool enableDepthTest = true;
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
			std::vector<VkFormat> renderTargetFormats;
		};
	}

	namespace Compute {
		class VulkanComputePipelineModule;
		class SPIRVComputeProgram;
	}

	namespace RayTracing {
		class VulkanRayTracingPipelineModule;
	}

	class VulkanPipelineFactory {
	public:
		VulkanPipelineFactory(VkDevice device);

		ref<Compute::VulkanComputePipelineModule> CreateComputePipeline(Material* material, std::string& error);
		ref<Compute::VulkanComputePipelineModule> CreateComputePipeline(const ShaderProgram* program, std::string& error);

		ref<Rasterization::VulkanRasterizationPipelineModule> CreateRasterizationPipeline(Material* material, VkRenderPass renderPass, const Rasterization::RasterizationPipelineProperties& properties, std::string& error);
		ref<Rasterization::VulkanRasterizationPipelineModule> CreateRasterizationPipeline(const ShaderProgram* program, VkRenderPass renderPass, const Rasterization::RasterizationPipelineProperties& properties, std::string& error);

		ref<MeshShading::VulkanMeshPipelineModule> CreateMeshPipeline(Material* material, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error);
		ref<MeshShading::VulkanMeshPipelineModule> CreateMeshPipeline(const ShaderProgram* program, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error);

		ref<RayTracing::VulkanRayTracingPipelineModule> CreateRayTracingPipeline(const ref<Material>& globalMaterial, const std::vector<ref<GameEntity>>& entities, std::string& error);

	private:
		ref<Compute::VulkanComputePipelineModule> CreateComputePipeline(const ShaderProgram* program, Material* material, std::string& error);

		struct GraphicsPipelineState {
			const std::vector<VkPipelineShaderStageCreateInfo>* stages = nullptr;
			VkPipelineLayout layout = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			const VkPipelineVertexInputStateCreateInfo* vertexInput = nullptr;
			const VkPipelineInputAssemblyStateCreateInfo* inputAssembly = nullptr;
			bool enableDepthTest = true;
			VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
			UInt32 renderTargetCount = 0;
		};

		VkPipeline CreateGraphicsPipeline(const GraphicsPipelineState& state, std::string& error);

		ref<MeshShading::VulkanMeshPipelineModule> CreateMeshPipeline(const ShaderProgram* program, Material* material, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error);

		ref<Rasterization::VulkanRasterizationPipelineModule> CreateRasterizationPipeline(const ShaderProgram* program, Material* material, VkRenderPass renderPass, const Rasterization::RasterizationPipelineProperties& properties, std::string& error);

	private:
		VkDevice m_device;
	};
}
