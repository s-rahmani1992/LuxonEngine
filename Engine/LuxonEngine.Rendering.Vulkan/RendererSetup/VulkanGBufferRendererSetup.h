#pragma once
#include "Core/VulkanRendererSetup.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanRenderTexture.h"
#include "Core/VulkanDepthTexture.h"
#include <vector>

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::Vulkan {
	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	namespace RayTracing {
		class VulkanRayTracingPipelineModule;
	}

	class VulkanGBufferRendererSetup : public IVulkanRendererSetup
	{
	public:
		virtual void Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context) override;
		virtual void Teardown() override;

	private:
		static constexpr UInt32 TargetCount = 3;
		static constexpr UInt32 Position = 0;
		static constexpr UInt32 Normal = 1;
		static constexpr UInt32 Mask = 2;

		bool SetupGBuffer(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context);
		void SetupRayTracing(VulkanPassSetupContext& context);
		void SetupShading(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context);

		static constexpr VkFormat s_formats[TargetCount] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32_UINT };

		std::array<std::array<float, 4>, TargetCount> m_clearColors = { {
			{ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } } };

		Ptr<VulkanRenderTexture> m_renderTextures[TargetCount];
		Ptr<VulkanDepthTexture> m_depthTexture;
		Ptr<VulkanRenderPass> m_gBufferPass;
		ref<Rasterization::VulkanRasterizationPipelineModule> m_gBufferRasterization;

		ref<Material> m_rayTracingMaterial;
		Ptr<VulkanTexture> m_rayTracingOutput;
		ref<RayTracing::VulkanRayTracingPipelineModule> m_rayTracingPipeline;

		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_pipelines;
	};
}
