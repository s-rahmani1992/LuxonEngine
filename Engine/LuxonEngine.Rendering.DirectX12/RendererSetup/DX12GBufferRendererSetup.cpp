#include "pch.h"
#include "DX12GBufferRendererSetup.h"
#include "Core/DX12PipelineFactory.h"
#include "Core/DX12GPUResourceManager.h"
#include "Core/DX12ComputePass.h"
#include "Rasterization/DX12RasterizationPipelineModule.h"
#include "RayTracing/DX12RayTracingPipelineModule.h"
#include "DX12MaterialFactory.h"
#include "DX12MeshController.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Logger.h>
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Rendering/Material.h>
#include <Rendering/RayTracingComponent.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::DX12 {
	void DX12GBufferRendererSetup::Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		Teardown();
		m_resourceManager = context.resourceManager;

		if (SetupGBuffer(entities, context) == false) {
			Logger::LogError("Failed to initialize the g buffer pipeline, the g buffer renderers are not drawn");
			Teardown();
			return;
		}

		context.gBufferRenderPass = m_gBufferRenderPass.get();

		context.globalDescriptors.push_back({ INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_renderTextures[Position]->GetSrvHandle() });
		context.globalDescriptors.push_back({ INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_renderTextures[Normal]->GetSrvHandle() });
		context.globalDescriptors.push_back({ INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_renderTextures[Mask]->GetSrvHandle() });

		SetupRayTracing(context);
		SetupShading(entities, context);
	}

	void DX12GBufferRendererSetup::Teardown()
	{
		// the passes and the pipelines point to the textures
		m_gBufferRenderPass = nullptr;
		m_gBufferRasterization = nullptr;
		m_gBufferMaterial = nullptr;
		m_rayTracingPipeline = nullptr;
		m_rayTracingMaterial = nullptr;

		if (m_resourceManager != nullptr) {
			for (auto& renderTexture : m_renderTextures)
				m_resourceManager->Destroy(renderTexture);

			m_resourceManager->Destroy(m_depthTexture);
			m_resourceManager->Destroy(m_rayTracingOutput);
		}
	}

	bool DX12GBufferRendererSetup::SetupGBuffer(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		m_gBufferMaterial = DX12MaterialFactory::BuildMaterial(context.getInternalProgram ? context.getInternalProgram("G_Buffer_Program") : nullptr);

		if (m_gBufferMaterial == nullptr)
			return false;

		for (UInt32 i = 0; i < TargetCount; i++) {
			m_renderTextures[i] = context.resourceManager->CreateRenderTexture(DX12TextureDesc{
				.width = context.renderWidth,
				.height = context.renderHeight,
				.format = m_formats[i],
				.clearValue = D3D12_CLEAR_VALUE{
					.Format = m_formats[i],
					.Color = { m_clearColors[i][0], m_clearColors[i][1], m_clearColors[i][2], m_clearColors[i][3] } },
				});

			if (m_renderTextures[i] == nullptr)
				return false;
		}

		m_depthTexture = context.resourceManager->CreateDepthTexture(DX12TextureDesc{
			.width = context.renderWidth,
			.height = context.renderHeight,
			.format = context.depthFormat,
			.clearValue = D3D12_CLEAR_VALUE{
				.Format = context.depthFormat,
				.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ .Depth = 1.0f, .Stencil = 0 } },
			});

		if (m_depthTexture == nullptr)
			return false;

		// the rasterization pipeline that draws into the g buffer. the formats of the pixel shader reflection do not describe the g buffer targets
		Rasterization::RasterizationPipelineProperties properties;
		properties.renderTargetFormats.assign(std::begin(m_formats), std::end(m_formats));

		std::string error;
		m_gBufferRasterization = context.pipelineFactory->CreateRasterizationPipeline(m_gBufferMaterial.get(), properties, error);

		if (m_gBufferRasterization == nullptr) {
			Logger::LogError("Failed to create the g buffer pipeline: " + error);
			return false;
		}

		if (m_gBufferRasterization->Initialize((UInt32)entities.size()) == false)
			return false;

		// phase 1: the renderers are drawn into the g buffer textures
		std::vector<DX12RenderTexture*> gBufferTargets;
		DX12RenderPassParams gBufferPassParams;
		gBufferPassParams.depthTarget = m_depthTexture.get();

		for (UInt32 i = 0; i < TargetCount; i++) {
			gBufferTargets.push_back(m_renderTextures[i].get());
			gBufferPassParams.clearColors.push_back({ m_clearColors[i][0], m_clearColors[i][1], m_clearColors[i][2], m_clearColors[i][3] });
		}

		m_gBufferRenderPass = std::make_unique<DX12RenderPass>();
		if (m_gBufferRenderPass->Initialize(gBufferTargets, gBufferPassParams) == false) {
			m_gBufferRenderPass = nullptr;
			return false;
		}
		m_gBufferRenderPass->AddPipeline(m_gBufferRasterization);

		// the camera view of the base context is in a non shader visible heap, so it can be the source of a descriptor copy
		m_gBufferRasterization->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);

		for (auto& rendererEntity : entities) {
			auto renderer = RendererCast<GBufferRTReflectionRenderer>(rendererEntity.renderer);

			if (renderer == nullptr || renderer->GetMesh() == nullptr)
				continue;

			auto meshController = std::dynamic_pointer_cast<DX12MeshController>(renderer->GetMesh()->GetGPUHandle());

			if (meshController == nullptr) // the mesh is not uploaded to the GPU, the entity is not drawn
				continue;

			m_gBufferRasterization->SetEntityDescriptor(rendererEntity.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, rendererEntity.gpuData->transformHeap->GetCPUDescriptorHandleForHeapStart());
			m_gBufferRasterization->SetEntityGeometry(rendererEntity.entity, *meshController->GetVertexView(), *meshController->GetIndexView(), meshController->GetMesh()->GetIndexCount());
		}

		// the third phase pipelines of the main render pass sample the g buffer textures
		for (auto& renderTexture : m_renderTextures)
			context.mainRenderPass->Use(renderTexture.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		return true;
	}

	void DX12GBufferRendererSetup::SetupRayTracing(DX12PassSetupContext& context)
	{
		// the output of the ray tracing phase. it is written as an unordered access view and sampled by the third phase
		m_rayTracingOutput = context.resourceManager->CreateTexture(DX12TextureDesc{
			.width = context.renderWidth,
			.height = context.renderHeight,
			.format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.name = L"Ray Tracing Output Texture",
			});

		m_rayTracingMaterial = DX12MaterialFactory::BuildMaterial(context.getInternalProgram ? context.getInternalProgram("G_Buffer_RT_Global_Program") : nullptr);

		if (m_rayTracingOutput == nullptr || m_rayTracingMaterial == nullptr) {
			Logger::LogError("Failed to create the resources of the ray tracing stage, the reflections are not traced");
			m_resourceManager->Destroy(m_rayTracingOutput);
			return;
		}

		float missColor[4] = { context.backgroundColor[0], context.backgroundColor[1], context.backgroundColor[2], context.backgroundColor[3] };
		m_rayTracingMaterial->SetValue("missColor", missColor, sizeof(missColor));

		// the pipeline is created with all the entities that have a ray tracing component at once
		std::vector<RayTracing::RayTracingEntityDesc> entityDescs;

		for (auto& entity : context.allEntities) {
			auto rtComponent = entity.gameEntity->GetRayTracingComponent();

			if (rtComponent != nullptr)
				entityDescs.push_back(RayTracing::RayTracingEntityDesc{ rtComponent.get(), entity.gameEntity.get() });
		}

		std::string error;
		m_rayTracingPipeline = context.pipelineFactory->CreateRayTracingPipeline(RayTracing::RayTracingPipelineProperties{}, m_rayTracingMaterial.get(), entityDescs, error);

		if (m_rayTracingPipeline == nullptr) {
			Logger::LogError("Failed to create the ray tracing pipeline: " + error);
			m_resourceManager->Destroy(m_rayTracingOutput);
			return;
		}

		m_rayTracingPipeline->SetDimensions(context.renderWidth, context.renderHeight);

		// the views are in non shader visible heaps, so they can be the source of the descriptor copies
		m_rayTracingPipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);
		m_rayTracingPipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightDescriptor);
		m_rayTracingPipeline->SetDescriptor(INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_renderTextures[Position]->GetSrvHandle());
		m_rayTracingPipeline->SetDescriptor(INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_renderTextures[Normal]->GetSrvHandle());
		m_rayTracingPipeline->SetDescriptor(INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_renderTextures[Mask]->GetSrvHandle());
		m_rayTracingPipeline->SetDescriptor(INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rayTracingOutput->GetUavHandle());

		for (auto& entity : context.allEntities) {
			auto rtComponent = entity.gameEntity->GetRayTracingComponent();

			if (rtComponent == nullptr || rtComponent->GetMesh() == nullptr)
				continue;

			auto meshController = std::dynamic_pointer_cast<DX12MeshController>(rtComponent->GetMesh()->GetGPUHandle());

			if (meshController == nullptr) // the mesh is not uploaded to the GPU, the entity is not traced
				continue;

			GameEntity* gameEntity = entity.gameEntity.get();
			m_rayTracingPipeline->SetEntityDescriptor(gameEntity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entity.transformHeap->GetCPUDescriptorHandleForHeapStart());
			m_rayTracingPipeline->SetEntityDescriptor(gameEntity, INTERNAL_VERTEX_BUFFER_NAME, meshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
			m_rayTracingPipeline->SetEntityDescriptor(gameEntity, INTERNAL_INDEX_BUFFER_NAME, meshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
		}

		// the compute pass writes the output and reads the g buffer, the third phase of the main render pass samples the output
		constexpr D3D12_RESOURCE_STATES gBufferReadState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		context.computePass->Use(m_rayTracingOutput.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context.mainRenderPass->Use(m_rayTracingOutput.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		for (auto& renderTexture : m_renderTextures)
			context.computePass->Use(renderTexture.get(), gBufferReadState);

		context.computePass->AddPipeline(m_rayTracingPipeline);

		context.globalDescriptors.push_back({ INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rayTracingOutput->GetSrvHandle() });
	}

	void DX12GBufferRendererSetup::SetupShading(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		struct ShadingEntityData {
			GameEntity* entity;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
			ref<DX12MeshController> meshController;
		};

		std::map<ref<Material>, std::vector<ShadingEntityData>> materialMap;

		// group the entities by their material, one pipeline is created per material
		for (auto& rendererEntity : entities) {
			auto renderer = RendererCast<GBufferRTReflectionRenderer>(rendererEntity.renderer);

			if (renderer == nullptr || renderer->GetMesh() == nullptr)
				continue;

			auto material = renderer->GetMaterial();

			if (material == nullptr) {
				Logger::LogWarning("The g buffer renderer of " + rendererEntity.entity->GetName() + " has no material and is not drawn");
				continue;
			}

			auto meshController = std::dynamic_pointer_cast<DX12MeshController>(renderer->GetMesh()->GetGPUHandle());
			if (meshController == nullptr) // the mesh is not uploaded to the GPU
				continue;

			materialMap[material].push_back(ShadingEntityData{
				.entity = rendererEntity.entity,
				.transformHandle = rendererEntity.gpuData->transformHeap->GetCPUDescriptorHandleForHeapStart(),
				.meshController = meshController,
				});
		}

		std::string error;
		Rasterization::RasterizationPipelineProperties properties;

		for (auto& [material, entityList] : materialMap) {
			auto pipeline = context.pipelineFactory->CreateRasterizationPipeline(material.get(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the rasterization pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)entityList.size()) == false)
				continue;

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightDescriptor);

			// the g buffer and the ray tracing output. a program that does not use them ignores them
			for (auto& globalDescriptor : context.globalDescriptors)
				pipeline->SetDescriptor(globalDescriptor.name, globalDescriptor.handle);

			for (auto& entityData : entityList) {
				pipeline->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformHandle);
				pipeline->SetEntityGeometry(entityData.entity, *entityData.meshController->GetVertexView(),
					*entityData.meshController->GetIndexView(), entityData.meshController->GetMesh()->GetIndexCount());
			}

			context.mainRenderPass->AddPipeline(pipeline);
		}
	}
}
