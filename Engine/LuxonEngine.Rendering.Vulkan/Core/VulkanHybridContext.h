#pragma once
#include "vulkan-pch.h"
#include "VulkanGraphicContext.h"
#include <map>
#include <vector>

namespace LuxonEngine {
	namespace Rendering
	{
		class SplineRenderer;
		class ShaderRegistery;
		class ShaderProgram;
		class Material;
	}
}

namespace LuxonEngine::Rendering::Vulkan {

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	namespace Compute {
		class VulkanComputePipelineModule;
	}

	namespace RayTracing {
		class SpirvrayTracingProgram;
		class VulkanRayTracingPipelineModule;
	}

	namespace MeshShading {
		class VulkanMeshPipelineModule;
	}

	class VulkanBuffer;
	class VulkanTexture;
	class VulkanRenderTexture;
	class VulkanDepthTexture;
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
		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;
		void CreateRasterizationModules(const std::map<ref<Material>, std::vector<VKEntityGPUData>>& entityMap,
			std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>>& modules);
		bool InitializeGBufferPass();
		bool CreateGBufferRenderPass();
		bool CreateGBufferTargets();
		void DestroyGBufferTargets();
		void RenderGBuffer(VkCommandBuffer commandBuffer);
		void CreateSplinePipelines(const std::vector<VKEntityGPUData>& splineEntities);
		void UpdateSplines();
		void DestroySplineBuffers();

		struct SplineGPUData {
			ref<SplineRenderer> renderer;
			GameEntity* entity = nullptr;
			UInt32 transformIndex = 0;
			UInt32 vertexCount = 0;
			Ptr<VulkanBuffer> vertexBuffer;
			ref<Rasterization::VulkanRasterizationPipelineModule> rasterModule;
		};

		// not owned. programs retrieved from it are wrapped in non-owning refs
		ShaderRegistery* m_shaderProgramRegistery;

		bool InitializeDepthBuffer();
		bool InitializeRenderPass();
		void UpdateEntityTransforms();

		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> CreateSpikeMeshPipelines();

		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> CreateSurfaceInstancePipelines();
		void DestroyMeshStorageBuffers();
		std::vector<EntityTransformBuffer> m_transformBuffers;
		TransformGPU m_transformData;

		VkRenderPass m_renderPass;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;

		VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
		Ptr<VulkanDepthTexture> m_depthTexture;

		std::vector<VKEntityGPUData> m_entityGPUList;
		std::vector<VKEntityGPUData> m_gBufferEntityGPUList;
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_rasterizationModules;
		std::vector<SplineGPUData> m_splines;
		ref<Compute::VulkanComputePipelineModule> m_splineComputeModule;

		enum GBufferTargetIndex {
			GBufferPosition,
			GBufferNormal,
			GBufferMask,
			GBufferTargetCount,
		};

		static constexpr VkFormat s_gBufferFormats[GBufferTargetCount] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32_UINT };

		VkRenderPass m_gBufferRenderPass = VK_NULL_HANDLE;
		VkFramebuffer m_gBufferFrameBuffer = VK_NULL_HANDLE;
		Ptr<VulkanRenderTexture> m_gBufferTargets[GBufferTargetCount];
		ref<Rasterization::VulkanRasterizationPipelineModule> m_gBufferPipeline;
		ref<RayTracing::VulkanRayTracingPipelineModule> m_rayTracingModule;
		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_gBufferRasterizationModules;

		Ptr<VulkanTexture> m_rtOutputTexture;

		VkClearValue m_clearValues[2];

		ref<VulkanPipelineFactory> m_pipelineFactory;
		std::vector<ref<MeshShading::VulkanMeshPipelineModule>> m_meshShadingPipelines;
		std::vector<VkBuffer> m_meshStorageBuffers; // storage buffer views of the spike meshes, they share the memory of the mesh buffers
	};
}