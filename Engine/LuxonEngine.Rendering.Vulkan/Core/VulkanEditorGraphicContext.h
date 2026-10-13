#pragma once
#include "VulkanGraphicContext.h"
#include "VulkanHybridContext.h"

namespace LuxonEngine {
	class GameEntity;
	class Scene;

	namespace Rendering {
		class Material;
		class ShaderRegistery;
		class ShaderProgram;
		class SplineRenderer;
	}
}

namespace LuxonEngine::Rendering::Vulkan {

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	namespace MeshShading {
		class VulkanMeshPipelineModule;
	}

	namespace Compute {
		class VulkanComputePipelineModule;
	}

	class VulkanPipelineFactory;

	class VulkanEditorGraphicContext : public VulkanGraphicContext
	{
	public:
		VulkanEditorGraphicContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window,
			const ref<VulkanPipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery);
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
		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> CreateSpikeMeshPipelines();

		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> CreateSurfaceInstancePipelines();
		void DestroyMeshStorageBuffers();
		void CreateSplinePipelines(const std::vector<VKEntityGPUData>& splineEntities);
		void UpdateSplines();
		void DestroySplineBuffers();
		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;

		struct SplineGPUData {
			ref<SplineRenderer> renderer;
			GameEntity* entity = nullptr;
			UInt32 transformIndex = 0;
			UInt32 vertexCount = 0;
			Ptr<VulkanBuffer> vertexBuffer;
			ref<Rasterization::VulkanRasterizationPipelineModule> rasterModule;
		};

		static ref<Mesh> ExtractMeshFromGameEntity(const ref<GameEntity>& entity);

	private:
		UInt32 m_transformStride;
		VkBuffer m_transformBuffer;
		VkDeviceMemory m_transformBufferMemory;
		TransformGPU m_transformData;

		VkRenderPass m_renderPass;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;

		VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
		Ptr<VulkanDepthTexture> m_depthTexture;

		std::vector<VKEntityGPUData> m_entityGPUList;
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_rasterizationModules;

		ref<Material> m_overrideMaterial;
		ref<Scene> m_scene;

		ShaderRegistery* m_shaderProgramRegistery;

		ref<VulkanPipelineFactory> m_pipelineFactory;
		std::vector<SplineGPUData> m_splines;
		ref<Compute::VulkanComputePipelineModule> m_splineComputeModule;
		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> m_meshShadingPipelines;
		std::vector<VkBuffer> m_meshStorageBuffers; // storage buffer views of the spike and surface instance meshes, they share the memory of the mesh buffers
	};
}