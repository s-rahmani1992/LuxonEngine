#pragma once
#include "VulkanGraphicContext.h"
#include "VulkanHybridContext.h"

namespace LuxonEngine {
	class GameEntity;
	class Scene;

	namespace Rendering {
		class Material;
	}
}

namespace LuxonEngine::Rendering::Vulkan {

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
		class VulkanRasterizationMaterial;
	}

	class VulkanEditorGraphicContext : public VulkanGraphicContext
	{
	public:
		VulkanEditorGraphicContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		~VulkanEditorGraphicContext();

		bool Initialize();
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;
		virtual void Resize(UInt32 width, UInt32 height) override;

	private:
		void UploadMeshesToGPU(const std::vector<ref<GameEntity>>& entities);
		bool InitializeDepthBuffer();
		bool InitializeRenderPass();
		void UpdateEntityTransforms();
		void InitializePipelines(const ref<Material>& overrideMaterial);
		void SyncEntities(const ref<Scene>& scene);

	private:
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
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_rasterizationModules;

		ref<Rasterization::VulkanRasterizationMaterial> m_sharedRasterMaterial;
		ref<Material> m_overrideMaterial;
		ref<Scene> m_scene;

		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	};
}