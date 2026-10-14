#pragma once
#include "vulkan-pch.h"
#include "VulkanGraphicContext.h"
#include "VulkanPassSequence.h"
#include "VulkanRendererSetupRegistry.h"
#include <vector>

namespace LuxonEngine {
	namespace Rendering
	{
		class ShaderRegistery;
		class ShaderProgram;
	}
}

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanDepthTexture;
	class VulkanRenderPass;
	class VulkanComputePass;
	class VulkanPipelineFactory;

	struct VKEntityGPUData {
	public:
		ref<GameEntity> gameEntity;
		UInt32 index;
	};

	class VulkanHybridContext : public VulkanGraphicContext {
	public:
		VulkanHybridContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window,
			const ref<VulkanPipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery);
		~VulkanHybridContext();
		bool Initialize();
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;
		virtual void Resize(UInt32 width, UInt32 height) override;
	private:

		void UploadMeshesToGPU(const std::vector<ref<GameEntity>>& entities);
		ShaderProgram* GetInternalProgram(const std::string& identifier) const;
		bool InitializePasses();
		void RunRendererSetups();
		bool BuildPassSequence();
		void DestroyMeshStorageBuffers();

		// not owned. programs retrieved from it are wrapped in non-owning refs
		ShaderRegistery* m_shaderProgramRegistery;

		bool InitializeDepthBuffer();
		bool InitializeRenderPass();
		void UpdateEntityTransforms();

		std::vector<EntityTransformBuffer> m_transformBuffers;
		TransformGPU m_transformData;

		Ptr<VulkanRenderPass> m_mainPass;
		Ptr<VulkanComputePass> m_computePass;
		VulkanPassSequence m_passSequence;

		VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
		Ptr<VulkanDepthTexture> m_depthTexture;

		std::vector<ref<GameEntity>> m_sceneEntities;
		std::vector<VKEntityGPUData> m_entityGPUList;
		std::vector<VulkanRendererEntity> m_rendererEntities;

		VulkanRendererSetupRegistry m_setupRegistry;
		VulkanPassSetupContext m_setupContext;

		VkClearValue m_clearColor = {};

		ref<VulkanPipelineFactory> m_pipelineFactory;
		std::vector<VkBuffer> m_meshStorageBuffers; // storage buffer views of the meshes, they share the memory of the mesh buffers
	};
}
