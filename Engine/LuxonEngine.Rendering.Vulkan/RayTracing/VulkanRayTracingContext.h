#pragma once
#include "vulkan-pch.h"
#include "Core/VulkanGraphicContext.h"

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class VulkanRayTracingPipelineModule;

	class VulkanRayTracingContext : public VulkanGraphicContext {
	public:
		VulkanRayTracingContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		virtual ~VulkanRayTracingContext();
		bool Initialize();
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		struct VKEntityGPUData {
		public:
			ref<GameEntity> gameEntity;
			UInt32 index;
		};

		void UploadMeshes(const std::vector<ref<GameEntity>>& entities);
		void UpdateTransforms();

		VkBuffer m_transformBuffer;
		VkDeviceMemory m_transformBufferMemory;
		TransformGPU m_transformData;

		VkImage m_outputImage;
		VkDeviceMemory m_outputImageMemory;
		VkImageView m_outputImageView;

		std::vector<VKEntityGPUData> m_entityGPUList;

		ref<RayTracing::VulkanRayTracingPipelineModule> m_rayTracingModule;
	};
}