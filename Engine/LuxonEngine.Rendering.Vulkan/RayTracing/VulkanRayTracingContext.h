#pragma once
#include "vulkan-pch.h"
#include "Core/VulkanGraphicContext.h"

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanPipelineFactory;
}

namespace LuxonEngine::Rendering::Vulkan::RayTracing {
	class VulkanRayTracingPipelineModule;

	class VulkanRayTracingContext : public VulkanGraphicContext {
	public:
		VulkanRayTracingContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window, const ref<VulkanPipelineFactory>& pipelineFactory);
		virtual ~VulkanRayTracingContext();
		bool Initialize();
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;
		virtual void Resize(UInt32 width, UInt32 height) override;
	private:
		struct VKEntityGPUData {
		public:
			ref<GameEntity> gameEntity;
			UInt32 index;
		};

		void UploadMeshes(const std::vector<ref<GameEntity>>& entities);
		void UpdateTransforms();
		void CreateOutputImage();
		void DestroyOutputImage();

		std::vector<EntityTransformBuffer> m_transformBuffers;
		std::vector<VkBuffer> m_meshStorageBuffers;
		TransformGPU m_transformData;

		VkImage m_outputImage;
		VkDeviceMemory m_outputImageMemory;
		VkImageView m_outputImageView;

		std::vector<VKEntityGPUData> m_entityGPUList;

		ref<VulkanPipelineFactory> m_pipelineFactory;
		ref<RayTracing::VulkanRayTracingPipelineModule> m_rayTracingModule;
	};
}