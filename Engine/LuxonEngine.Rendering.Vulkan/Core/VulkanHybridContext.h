#pragma once
#include "vulkan-pch.h"
#include "VulkanGraphicContext.h"

namespace QuantumEngine {
	namespace Rendering
	{
		class SplineRenderer;
	}
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanSplinePipelineModule;
	class VulkanGBufferPipelineModule;

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	namespace RayTracing {
		class SpirvrayTracingProgram;
		class VulkanRayTracingPipelineModule;
	}

	struct VKEntityGPUData {
	public:
		ref<GameEntity> gameEntity;
		UInt32 index;
	};

	class VulkanHybridContext : public VulkanGraphicContext {
	public:
		VulkanHybridContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		~VulkanHybridContext();
		bool Initialize();
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;
	private:

		

		void UploadMeshesToGPU(const std::vector<ref<GameEntity>>& entities);
		bool InitializeDepthBuffer();
		bool InitializeRenderPass();
		void UpdateEntityTransforms();

		UInt32 m_transformStride;
		VkBuffer m_transformBuffer;
		VkDeviceMemory m_transformBufferMemory;
		TransformGPU m_transformData;

		VkRenderPass m_renderPass;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;

		VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
		VkImage m_depthImage;
		VkDeviceMemory m_depthMemory;
		VkImageView m_depthImageView;

		std::vector<VKEntityGPUData> m_entityGPUList;
		std::vector<VKEntityGPUData> m_gBufferEntityGPUList;
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_rasterizationModules;
		std::vector<ref<VulkanSplinePipelineModule>> m_splineModues;

		ref<VulkanGBufferPipelineModule> m_gbufferModule = nullptr;
		ref<RayTracing::VulkanRayTracingPipelineModule> m_rayTracingModule;
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_gBufferRasterizationModules;

		VkImage m_rtOutputImage;
		VkDeviceMemory m_rtOutputImageMemory;
		VkImageView m_rtOutputImageView;

		VkDescriptorPool m_descriptorPool;
	};
}