#include "vulkan-pch.h"
#include "VulkanGBufferRendererSetup.h"
#include "Core/VulkanPipelineFactory.h"
#include "Core/VulkanBufferFactory.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanUtilities.h"
#include "Core/VulkanComputePass.h"
#include "Core/VulkanMeshController.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "RayTracing/VulkanRayTracingPipelineModule.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Logger.h>
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Rendering/Material.h>
#include <Rendering/MaterialFactory.h>
#include <Rendering/RayTracingComponent.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanGBufferRendererSetup::Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		Teardown();

		if (SetupGBuffer(entities, context) == false) {
			Logger::LogError("Failed to initialize the g buffer pipeline, the g buffer renderers are not drawn");
			Teardown();
			return;
		}

		context.gBufferRenderPass = m_gBufferPass.get();

		context.globalDescriptors.push_back({ INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_renderTextures[Position]->GetView() });
		context.globalDescriptors.push_back({ INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_renderTextures[Normal]->GetView() });
		context.globalDescriptors.push_back({ INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_renderTextures[Mask]->GetView() });

		SetupRayTracing(context);
		SetupShading(entities, context);
	}

	void VulkanGBufferRendererSetup::Teardown()
	{
		m_pipelines.clear();
		m_rayTracingPipeline.reset();
		m_rayTracingMaterial.reset();
		m_gBufferRasterization.reset();
		m_gBufferPass.reset();

		m_rayTracingOutput.reset();
		m_depthTexture.reset();

		for (auto& renderTexture : m_renderTextures)
			renderTexture.reset();
	}

	bool VulkanGBufferRendererSetup::SetupGBuffer(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		auto* gBufferProgram = context.getInternalProgram ? context.getInternalProgram("G_Buffer_Program") : nullptr;

		if (gBufferProgram == nullptr)
			return false;

		std::vector<VulkanRenderTexture*> gBufferTargets;
		VulkanRenderPassParams gBufferPassParams;

		for (UInt32 i = 0; i < TargetCount; i++) {
			m_renderTextures[i] = context.bufferFactory->CreateRenderTexture(VulkanTextureDesc{
				.width = context.renderExtent.width,
				.height = context.renderExtent.height,
				.format = s_formats[i],
				.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			});

			if (m_renderTextures[i] == nullptr)
				return false;

			gBufferTargets.push_back(m_renderTextures[i].get());
			gBufferPassParams.clearColors.push_back(m_clearColors[i]);
		}

		m_depthTexture = context.bufferFactory->CreateDepthTexture(VulkanTextureDesc{
			.width = context.renderExtent.width,
			.height = context.renderExtent.height,
			.format = context.depthFormat,
		});

		if (m_depthTexture == nullptr)
			return false;

		gBufferPassParams.depthTarget = m_depthTexture.get();
		gBufferPassParams.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		m_gBufferPass = std::make_unique<VulkanRenderPass>();

		if (m_gBufferPass->Initialize(context.device, gBufferTargets, gBufferPassParams) == false)
			return false;

		Rasterization::RasterizationPipelineProperties properties;
		properties.renderTargetFormats.assign(std::begin(s_formats), std::end(s_formats));

		std::string error;
		m_gBufferRasterization = context.pipelineFactory->CreateRasterizationPipeline(gBufferProgram, m_gBufferPass->GetRenderPass(), properties, error);

		if (m_gBufferRasterization == nullptr) {
			Logger::LogError("Failed to create the g buffer pipeline: " + error);
			return false;
		}

		if (m_gBufferRasterization->Initialize((UInt32)entities.size()) == false) {
			Logger::LogError("Failed to create the descriptors of the g buffer pipeline");
			return false;
		}

		m_gBufferRasterization->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer, 0, context.cameraStride);

		for (auto& rendererEntity : entities) {
			auto* renderer = RendererCast<GBufferRTReflectionRenderer>(rendererEntity.renderer);

			if (renderer == nullptr || renderer->GetMesh() == nullptr)
				continue;

			auto mesh = renderer->GetMesh();
			auto* meshController = dynamic_cast<VulkanMeshController*>(mesh->GetGPUHandle().get());

			if (meshController == nullptr)
				continue;

			m_gBufferRasterization->SetEntityDescriptor(rendererEntity.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, rendererEntity.transformBuffer);
			m_gBufferRasterization->SetEntityGeometry(rendererEntity.entity, meshController->GetVertexBuffer(), meshController->GetIndexBuffer(), mesh->GetIndexCount());
		}

		m_gBufferPass->AddPipeline(m_gBufferRasterization.get());

		for (auto& renderTexture : m_renderTextures)
			context.mainRenderPass->Use(renderTexture.get(), VulkanStates::Sampled(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));

		return true;
	}

	void VulkanGBufferRendererSetup::SetupRayTracing(VulkanPassSetupContext& context)
	{
		m_rayTracingOutput = context.bufferFactory->CreateTexture(VulkanTextureDesc{
			.width = context.renderExtent.width,
			.height = context.renderExtent.height,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		});

		auto* globalProgram = context.getInternalProgram ? context.getInternalProgram("G_Buffer_RT_Global_Program") : nullptr;

		if (m_rayTracingOutput == nullptr || globalProgram == nullptr) {
			Logger::LogError("Failed to create the resources of the ray tracing stage, the reflections are not traced");
			m_rayTracingOutput.reset();
			return;
		}

		auto materialFactory = VulkanDeviceManager::Instance()->CreateMaterialFactory();
		m_rayTracingMaterial = materialFactory->CreateMaterial(ref<ShaderProgram>(globalProgram, [](ShaderProgram*) {}));

		if (m_rayTracingMaterial == nullptr) {
			Logger::LogError("Failed to create the material of the ray tracing stage, the reflections are not traced");
			m_rayTracingOutput.reset();
			return;
		}

		m_rayTracingMaterial->SetValue("missColor", context.backgroundColor.data(), (UInt32)(context.backgroundColor.size() * sizeof(Float)));

		std::string error;
		m_rayTracingPipeline = context.pipelineFactory->CreateRayTracingPipeline(m_rayTracingMaterial, *context.sceneEntities, error);

		if (m_rayTracingPipeline == nullptr) {
			Logger::LogError("Failed to create the ray tracing pipeline: " + error);
			m_rayTracingMaterial.reset();
			m_rayTracingOutput.reset();
			return;
		}

		m_rayTracingPipeline->SetExtent(context.renderExtent);
		m_rayTracingPipeline->SetBuffer(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer);
		m_rayTracingPipeline->SetBuffer(INTERNAL_LIGHT_DATA_NAME, context.lightBuffer);

		for (auto& rendererEntity : context.allEntities) {
			auto rtComponent = rendererEntity.entity->GetRayTracingComponent();

			if (rtComponent == nullptr || rtComponent->GetMesh() == nullptr)
				continue;

			auto* meshController = dynamic_cast<VulkanMeshController*>(rtComponent->GetMesh()->GetGPUHandle().get());

			if (meshController == nullptr)
				continue;

			VkBuffer vertexStorageBuffer = meshController->CreateVertexStorageBuffer();
			VkBuffer indexStorageBuffer = meshController->CreateIndexStorageBuffer();
			context.meshStorageBuffers->push_back(vertexStorageBuffer);
			context.meshStorageBuffers->push_back(indexStorageBuffer);

			m_rayTracingPipeline->SetEntityBuffer(rendererEntity.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, rendererEntity.transformBuffer);
			m_rayTracingPipeline->SetEntityBuffer(rendererEntity.entity, INTERNAL_VERTEX_BUFFER_NAME, vertexStorageBuffer);
			m_rayTracingPipeline->SetEntityBuffer(rendererEntity.entity, INTERNAL_INDEX_BUFFER_NAME, indexStorageBuffer);
		}

		m_rayTracingPipeline->SetImage("_PositionTexture", m_renderTextures[Position]->GetView());
		m_rayTracingPipeline->SetImage("_NormalTexture", m_renderTextures[Normal]->GetView());
		m_rayTracingPipeline->SetImage("_MaskTexture", m_renderTextures[Mask]->GetView());
		m_rayTracingPipeline->SetImage(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rayTracingOutput->GetView());

		constexpr VkPipelineStageFlags rayTracingStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

		context.computePass->Use(m_rayTracingOutput.get(), VulkanStates::Storage(rayTracingStage));
		context.mainRenderPass->Use(m_rayTracingOutput.get(), VulkanStates::Sampled(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));

		for (auto& renderTexture : m_renderTextures)
			context.computePass->Use(renderTexture.get(), VulkanStates::Storage(rayTracingStage));

		context.computePass->AddPipeline(m_rayTracingPipeline.get());

		context.globalDescriptors.push_back({ INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rayTracingOutput->GetView() });
	}

	void VulkanGBufferRendererSetup::SetupShading(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		struct ShadingEntityData {
			GameEntity* entity;
			VkBuffer transformBuffer;
			VulkanMeshController* meshController;
			UInt32 indexCount;
		};

		std::map<Material*, std::vector<ShadingEntityData>> materialMap;

		for (auto& rendererEntity : entities) {
			auto* renderer = RendererCast<GBufferRTReflectionRenderer>(rendererEntity.renderer);

			if (renderer == nullptr || renderer->GetMesh() == nullptr)
				continue;

			auto material = renderer->GetMaterial();

			if (material == nullptr) {
				Logger::LogWarning("The g buffer renderer of " + rendererEntity.entity->GetName() + " has no material and is not drawn");
				continue;
			}

			auto mesh = renderer->GetMesh();
			auto* meshController = dynamic_cast<VulkanMeshController*>(mesh->GetGPUHandle().get());

			if (meshController == nullptr)
				continue;

			materialMap[material.get()].push_back(ShadingEntityData{
				.entity = rendererEntity.entity,
				.transformBuffer = rendererEntity.transformBuffer,
				.meshController = meshController,
				.indexCount = mesh->GetIndexCount(),
				});
		}

		std::string error;
		Rasterization::RasterizationPipelineProperties properties;

		for (auto& [material, entityList] : materialMap) {
			auto pipeline = context.pipelineFactory->CreateRasterizationPipeline(material, context.mainRenderPass->GetRenderPass(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the rasterization pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)entityList.size()) == false) {
				Logger::LogError("Failed to create the descriptors of the rasterization pipeline");
				continue;
			}

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer, 0, context.cameraStride);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightBuffer, 0, context.lightStride);

			for (auto& globalDescriptor : context.globalDescriptors)
				pipeline->SetDescriptor(globalDescriptor.name, globalDescriptor.imageView);

			for (auto& entityData : entityList) {
				pipeline->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformBuffer);
				pipeline->SetEntityGeometry(entityData.entity, entityData.meshController->GetVertexBuffer(), entityData.meshController->GetIndexBuffer(), entityData.indexCount);
			}

			context.mainRenderPass->AddPipeline(pipeline.get());
			m_pipelines.push_back(pipeline);
		}
	}
}
