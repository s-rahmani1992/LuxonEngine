#pragma once
#include "Core/DX12RendererSetup.h"
#include "Core/DX12RenderPass.h"
#include "Core/DX12RenderTexture.h"
#include "Core/DX12DepthTexture.h"

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::DX12 {
	namespace Rasterization {
		class DX12RasterizationPipelineModule;
	}

	namespace RayTracing {
		class DX12RayTracingPipelineModule;
	}

	// The reflection renderer, drawn in three phases:
	//  1. the renderers are drawn into the g buffer (position, normal, mask) by a render pass of their own
	//  2. the reflections are traced from the g buffer into the ray tracing output, by the compute pass
	//  3. the renderers are drawn into the main render pass, with the g buffer and the ray tracing output bound by name
	// The g buffer textures and the ray tracing output are added to the global descriptors of the context, so this setup has to be
	// registered before the setups whose programs read them.
	class DX12GBufferRendererSetup : public IDX12RendererSetup
	{
	public:
		virtual void Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context) override;
		virtual void Teardown() override;

	private:
		static constexpr UInt32 TargetCount = 3;
		static constexpr UInt32 Position = 0;
		static constexpr UInt32 Normal = 1;
		static constexpr UInt32 Mask = 2;

		bool SetupGBuffer(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context);
		void SetupRayTracing(DX12PassSetupContext& context);
		void SetupShading(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context);

		DX12GPUResourceManager* m_resourceManager = nullptr;

		// The clear colors are also the optimized clear values of the textures. Clearing to another color makes D3D12 warn
		// (CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE) and is slower.
		float m_clearColors[TargetCount][4] = { { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
		DXGI_FORMAT m_formats[TargetCount] = { DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R8_UINT };

		Ptr<DX12RenderTexture> m_renderTextures[TargetCount] = {};
		Ptr<DX12DepthTexture> m_depthTexture;
		ref<Material> m_gBufferMaterial;
		ref<Rasterization::DX12RasterizationPipelineModule> m_gBufferRasterization; // draws the renderers into the g buffer
		Ptr<DX12RenderPass> m_gBufferRenderPass; // it points to the textures above

		ref<Material> m_rayTracingMaterial; // the global material of the ray tracing pipeline, that points to it
		Ptr<DX12Texture> m_rayTracingOutput; // written by the ray tracing phase, sampled by the third phase
		ref<RayTracing::DX12RayTracingPipelineModule> m_rayTracingPipeline;
	};
}
