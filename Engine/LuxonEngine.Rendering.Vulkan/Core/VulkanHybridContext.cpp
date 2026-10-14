#include "vulkan-pch.h"
#include "VulkanHybridContext.h"
#include "Core/Scene.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "Core/Transform.h"
#include "Rendering/Renderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/SplineRenderer.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include "Rendering/RayTracingComponent.h"
#include "VulkanAssetManager.h"
#include "VulkanUtilities.h"
#include <Rendering/ShaderRegistery.h>
#include "VulkanBufferFactory.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "Compute/VulkanComputePipelineModule.h"
#include "VulkanSplineVertex.h"
#include "Compute/SPIRVComputeProgram.h"
#include "RayTracing/VulkanRayTracingPipelineModule.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanMaterialFactory.h"
#include "Core/Logger.h"
#include "Core/VulkanPipelineFactory.h"
#include "Core/VulkanMeshController.h"
#include "Mesh/VulkanMeshPipelineModule.h"
#include "Rendering/SpikeMeshRenderer.h"
#include "Rendering/SurfaceInstanceRenderer.h"
#include "Core/VulkanTexture2DController.h"
#include "VulkanRenderPass.h"
#include "VulkanComputePass.h"
#include "VulkanRenderTexture.h"
#include "VulkanDepthTexture.h"
#include "Core/Texture2D.h"
#include "Rendering/ShaderInternalNames.h"
#include <map>

LuxonEngine::Rendering::Vulkan::VulkanHybridContext::VulkanHybridContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window,
	const ref<VulkanPipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery)
	:VulkanGraphicContext(vkInstance, surfaceQueueFamilyIndex, window), m_shaderProgramRegistery(shaderRegistery), m_pipelineFactory(pipelineFactory)
{
	m_swapChainUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

LuxonEngine::Rendering::Vulkan::VulkanHybridContext::~VulkanHybridContext()
{
	// the pipelines are destroyed before the buffers they use
	m_meshShadingPipelines.clear();
	DestroyMeshStorageBuffers();

	m_rasterizationModules.clear();
	m_gBufferRasterizationModules.clear();
	m_computePass.reset();
	m_splineComputeModule.reset();
	DestroySplineBuffers();

	DestroyEntityTransformBuffers(m_transformBuffers);
	m_gBufferPipeline.reset();
	m_gBufferPass.reset();
	DestroyGBufferTargets();

	DestroyEntityTransformBuffers(m_transformBuffers);

	m_mainPass.reset();
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::Initialize()
{
	if (InitializeSurface() == false)
		return false;

	if (InitializeSwapChain(m_swapChainUsageFlags) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	if(InitializeRenderPass() == false)
		return false;

	if(InitializeCommandObjects() == false)
		return false;

	if(InitializeFencesAndSemaphores() == false)
		return false;

	m_rtOutputTexture = m_bufferFactory->CreateTexture(VulkanTextureDesc{
		.width = m_swapChainCapability.currentExtent.width,
		.height = m_swapChainCapability.currentExtent.height,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	});

	if (m_rtOutputTexture == nullptr)
		return false;

	return true;
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::PrepareScene(const ref<Scene>& scene)
{
	UploadMeshesToGPU(scene->entities);

	if(InitializeCameraBuffer(scene->mainCamera) == false)
		return false;

	if(InitializeLightBuffer(scene->lightData) == false)
		return false;

	auto colorArray = scene->hybridBackgroundColor.GetColorArray();
	m_clearColor.color = { colorArray[0], colorArray[1], colorArray[2], colorArray[3] };

	if (InitializeRenderPass() == false)
		return false;
	
	// Create Uniform Buffers for Transforms
	if (CreateEntityTransformBuffers((UInt32)scene->entities.size(), m_transformBuffers) == false)
		return false;


	std::map<ref<Material>, std::vector<VKEntityGPUData>> rasterizationEntityMap;
	std::map<ref<Material>, std::vector<VKEntityGPUData>> gBufferEntityMap;
	std::vector<VKEntityGPUData> splineEntities;
	UInt32 index = 0;
	m_entityGPUList.reserve(scene->entities.size());

	for (auto& entity : scene->entities) {
		m_entityGPUList.push_back({ entity, index });
		index++;

	}

	for (auto& entityGPU : m_entityGPUList) {
		auto& entity = entityGPU.gameEntity;
		auto Renderer = entity->GetRenderer();

		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(Renderer);

		if (meshRenderer != nullptr) {
			rasterizationEntityMap[meshRenderer->GetMaterial()].push_back(entityGPU);
			continue;
		}

		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(Renderer);

		if (splineRenderer != nullptr) {
			splineEntities.push_back(entityGPU);
			continue;
		}

		auto gBufferRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(Renderer);

		if (gBufferRenderer != nullptr) {
			m_gBufferEntityGPUList.push_back(entityGPU);

			gBufferEntityMap[gBufferRenderer->GetMaterial()].push_back(entityGPU);
		}
	}

	CreateRasterizationModules(rasterizationEntityMap, m_rasterizationModules);
	CreateRasterizationModules(gBufferEntityMap, m_gBufferRasterizationModules);
	CreateSplinePipelines(splineEntities);

	m_computePass = std::make_unique<VulkanComputePass>();
	m_computePass->AddPipeline(m_splineComputeModule.get());

	m_meshShadingPipelines = CreateSpikeMeshPipelines();

	auto surfaceInstancePipelines = CreateSurfaceInstancePipelines();
	m_meshShadingPipelines.insert(m_meshShadingPipelines.end(), surfaceInstancePipelines.begin(), surfaceInstancePipelines.end());

	for (auto& module : m_rasterizationModules)
		m_mainPass->AddPipeline(module.get());

	for (auto& module : m_gBufferRasterizationModules)
		m_mainPass->AddPipeline(module.get());

	for (auto& meshShadingPipeline : m_meshShadingPipelines)
		m_mainPass->AddPipeline(meshShadingPipeline.get());

	if (m_gBufferEntityGPUList.size() > 0) {
		auto gBufferProgram = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>( GetInternalProgram("G_Buffer_Program"));
		if (InitializeGBufferPass() == false)
			return false;

		auto gBufferGlobalProgram = GetInternalProgram("G_Buffer_RT_Global_Program");

		auto materialFactory = VulkanDeviceManager::Instance()->CreateMaterialFactory();
		auto gBufferGlobalMaterial = materialFactory->CreateMaterial(gBufferGlobalProgram);
		gBufferGlobalMaterial->SetValue("missColor", &m_clearColor, 4 * sizeof(Float));
		std::string rayTracingError;
		m_rayTracingModule = m_pipelineFactory->CreateRayTracingPipeline(gBufferGlobalMaterial, scene->entities, rayTracingError);

		if (m_rayTracingModule == nullptr) {
			Logger::LogError("Failed to create the ray tracing pipeline: " + rayTracingError);
			return false;
		}

		m_computePass->AddPipeline(m_rayTracingModule.get());

		m_rayTracingModule->SetExtent(m_swapChainCapability.currentExtent);

		BindRayTracingBuffers(*m_rayTracingModule, scene->entities, m_transformBuffers, m_meshStorageBuffers);

		m_rayTracingModule->SetImage("_PositionTexture", m_gBufferTargets[GBufferPosition]->GetView());
		m_rayTracingModule->SetImage("_NormalTexture", m_gBufferTargets[GBufferNormal]->GetView());
		m_rayTracingModule->SetImage("_MaskTexture", m_gBufferTargets[GBufferMask]->GetView());
		m_rayTracingModule->SetImage(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputTexture->GetView());

		for (auto& rasterModule : m_gBufferRasterizationModules) {
			rasterModule->SetDescriptor(INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_gBufferTargets[GBufferPosition]->GetView());
			rasterModule->SetDescriptor(INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_gBufferTargets[GBufferNormal]->GetView());
			rasterModule->SetDescriptor(INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_gBufferTargets[GBufferMask]->GetView());
			rasterModule->SetDescriptor(INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputTexture->GetView());
		}
	}

	return true;
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::Render()
{
	UpdateCameraBuffer();
	UpdateEntityTransforms();

	vkResetFences(m_logicDevice, 1, &m_fence);

	UInt32 imageIndex = 0;
	VkResult acquireResult = vkAcquireNextImageKHR(m_logicDevice, m_swapChain, UINT64_MAX,
		m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
		Resize(m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height);
		return; // skip this frame
	}
	if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
		// log error and bail out
		return;
	}
	vkResetCommandBuffer(m_commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr,
	};

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	if (m_splineComputeModule != nullptr)
		UpdateSplines();

	if (m_gBufferPipeline != nullptr) {
		m_gBufferPass->Execute(m_commandBuffer);

		// Transition G-Buffer images from SHADER_READ_ONLY_OPTIMAL -> GENERAL for ray tracing usage.
		// The ray tracing descriptors were created with VK_IMAGE_LAYOUT_GENERAL, so we must match that layout before tracing.
		VkImageMemoryBarrier gBufferBarriers[3]{};

		// Position image
		gBufferBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		gBufferBarriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		gBufferBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		gBufferBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		gBufferBarriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
		gBufferBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		gBufferBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		gBufferBarriers[0].image = m_gBufferTargets[GBufferPosition]->GetImage();
		gBufferBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		gBufferBarriers[0].subresourceRange.baseMipLevel = 0;
		gBufferBarriers[0].subresourceRange.levelCount = 1;
		gBufferBarriers[0].subresourceRange.baseArrayLayer = 0;
		gBufferBarriers[0].subresourceRange.layerCount = 1;

		// Normal image
		gBufferBarriers[1] = gBufferBarriers[0];
		gBufferBarriers[1].image = m_gBufferTargets[GBufferNormal]->GetImage();

		// Mask image
		gBufferBarriers[2] = gBufferBarriers[0];
		gBufferBarriers[2].image = m_gBufferTargets[GBufferMask]->GetImage();

		vkCmdPipelineBarrier(
			m_commandBuffer,
			VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			0,
			0, nullptr,
			0, nullptr,
			3, gBufferBarriers);

		// Prepare ray tracing output image for writes
		VkImageMemoryBarrier rtOutImageBarrier{};
		rtOutImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		rtOutImageBarrier.srcAccessMask = 0;
		rtOutImageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		rtOutImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		rtOutImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		rtOutImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		rtOutImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		rtOutImageBarrier.image = m_rtOutputTexture->GetImage();
		rtOutImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		rtOutImageBarrier.subresourceRange.baseMipLevel = 0;
		rtOutImageBarrier.subresourceRange.levelCount = 1;
		rtOutImageBarrier.subresourceRange.baseArrayLayer = 0;
		rtOutImageBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			0, 0, nullptr, 0, nullptr, 1, &rtOutImageBarrier);


		m_computePass->Execute(m_commandBuffer);

		// Transition ray tracing output to SHADER_READ_ONLY_OPTIMAL for later sampling in rasterization
		rtOutImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		rtOutImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		rtOutImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		rtOutImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			0, 0, nullptr, 0, nullptr, 1, &rtOutImageBarrier);

		// Transition G-Buffer images back from GENERAL -> SHADER_READ_ONLY_OPTIMAL
		// so further graphics/fragment sampling sees the expected layout.
		for (int i = 0; i < 3; ++i) {
			gBufferBarriers[i].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			gBufferBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			gBufferBarriers[i].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			gBufferBarriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		vkCmdPipelineBarrier(
			m_commandBuffer,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			3, gBufferBarriers);

	}
	else {
		m_computePass->Execute(m_commandBuffer);
	}

	if (m_splineComputeModule != nullptr) {
		VkMemoryBarrier splineBarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
		};

		vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0, 1, &splineBarrier, 0, nullptr, 0, nullptr);
	}

	m_mainPass->SetImageIndex(imageIndex);
	m_mainPass->Execute(m_commandBuffer);
	vkEndCommandBuffer(m_commandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_imageAvailableSemaphore,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &m_renderFinishedSemaphore,
	};

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_fence);

	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_renderFinishedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapChain,
		.pImageIndices = &imageIndex,
	};

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	vkQueueWaitIdle(m_presentQueue);

	vkWaitForFences(m_logicDevice, 1, &m_fence, VK_TRUE, 20000);
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::Resize(UInt32 width, UInt32 height)
{
	m_depthTexture.reset();

	RecreateSwapChain(m_swapChainUsageFlags);

	InitializeDepthBuffer();

	InitializeRenderPass();

	if (m_gBufferPipeline != nullptr) {
		DestroyGBufferTargets();
		CreateGBufferTargets();
	}

	if (m_rayTracingModule != nullptr) {
		m_rtOutputTexture = m_bufferFactory->CreateTexture(VulkanTextureDesc{
			.width = m_swapChainCapability.currentExtent.width,
			.height = m_swapChainCapability.currentExtent.height,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		});

		m_rayTracingModule->SetExtent(m_swapChainCapability.currentExtent);
		m_rayTracingModule->SetImage(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputTexture->GetView());
	
		if (m_gBufferPipeline != nullptr) {
			m_rayTracingModule->SetImage("_PositionTexture", m_gBufferTargets[GBufferPosition]->GetView());
			m_rayTracingModule->SetImage("_NormalTexture", m_gBufferTargets[GBufferNormal]->GetView());
			m_rayTracingModule->SetImage("_MaskTexture", m_gBufferTargets[GBufferMask]->GetView());
		
		
			for (auto& rasterModule : m_gBufferRasterizationModules) {
				rasterModule->SetDescriptor(INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_gBufferTargets[GBufferPosition]->GetView());
				rasterModule->SetDescriptor(INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_gBufferTargets[GBufferNormal]->GetView());
				rasterModule->SetDescriptor(INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_gBufferTargets[GBufferMask]->GetView());
				rasterModule->SetDescriptor(INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputTexture->GetView());
			}
		}
	}
}
void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::UploadMeshesToGPU(const std::vector<ref<GameEntity>>& entities)
{
	std::set<ref<Mesh>> uniqueMeshes;

	for (auto& entity : entities) {
		auto mesh = entity->GetRenderer()->GetMesh();

		if (mesh == nullptr)
			continue;

		uniqueMeshes.insert(mesh);
		auto rtcomponent = entity->GetRayTracingComponent();

		if (rtcomponent != nullptr)
			uniqueMeshes.insert(rtcomponent->GetMesh());
	}

	m_assetManager->UploadMeshesToGPU(std::vector<ref<Mesh>>(uniqueMeshes.begin(), uniqueMeshes.end()));
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::InitializeDepthBuffer()
{
	m_depthTexture = m_bufferFactory->CreateDepthTexture(VulkanTextureDesc{
		.width = m_swapChainCapability.currentExtent.width,
		.height = m_swapChainCapability.currentExtent.height,
		.format = m_depthFormat,
	});

	return m_depthTexture != nullptr;
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::InitializeRenderPass()
{
	if (m_mainPass == nullptr)
		m_mainPass = std::make_unique<VulkanRenderPass>();

	VulkanRenderPassParams params;
	params.clearColors = { { m_clearColor.color.float32[0], m_clearColor.color.float32[1], m_clearColor.color.float32[2], m_clearColor.color.float32[3] } };
	params.depthTarget = m_depthTexture.get();
	params.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	return m_mainPass->Initialize(m_logicDevice, m_swapChainFormat.format, m_swapChainCapability.currentExtent, m_swapChainImageViews, params);
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::UpdateEntityTransforms()
{
	for (auto& entityGPU : m_entityGPUList) {
		m_transformData.modelMatrix = entityGPU.gameEntity->GetTransform()->Matrix();
		m_transformData.modelViewMatrix = m_cameraGPU.viewMatrix * m_transformData.modelMatrix;
		m_transformData.rotationMatrix = entityGPU.gameEntity->GetTransform()->RotateMatrix();

		std::memcpy(m_transformBuffers[entityGPU.index].mapped, &m_transformData, sizeof(TransformGPU));
	}
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::CreateRasterizationModules(const std::map<ref<Material>, std::vector<VKEntityGPUData>>& entityMap,
	std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>>& modules)
{
	std::string error;
	Rasterization::RasterizationPipelineProperties properties;

	for (auto& [material, entityList] : entityMap) {
		auto rasterizationModule = m_pipelineFactory->CreateRasterizationPipeline(material.get(), m_mainPass->GetRenderPass(), properties, error);

		if (rasterizationModule == nullptr) {
			Logger::LogError("Failed to create the rasterization pipeline: " + error);
			continue;
		}

		if (rasterizationModule->Initialize((UInt32)entityList.size()) == false) {
			Logger::LogError("Failed to create the descriptors of the rasterization pipeline");
			continue;
		}

		rasterizationModule->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraBuffer, 0, m_cameraStride);
		rasterizationModule->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightBuffer, 0, m_lightStride);

		for (auto& entityGPU : entityList) {
			auto mesh = entityGPU.gameEntity->GetRenderer()->GetMesh();

			if (mesh == nullptr)
				continue;

			auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(mesh->GetGPUHandle());

			if (meshController == nullptr)
				continue;

			GameEntity* entity = entityGPU.gameEntity.get();

			rasterizationModule->SetEntityDescriptor(entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffers[entityGPU.index].buffer);
			rasterizationModule->SetEntityGeometry(entity, meshController->GetVertexBuffer(), meshController->GetIndexBuffer(), mesh->GetIndexCount());
		}

		modules.push_back(rasterizationModule);
	}
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::InitializeGBufferPass()
{
	auto gBufferProgram = GetInternalProgram("G_Buffer_Program");

	if (gBufferProgram == nullptr) {
		Logger::LogError("The g buffer program is not registered");
		return false;
	}

	m_gBufferPass = std::make_unique<VulkanRenderPass>();

	if (CreateGBufferTargets() == false) {
		Logger::LogError("Failed to create the render targets of the g buffer");
		return false;
	}

	Rasterization::RasterizationPipelineProperties properties;
	properties.renderTargetFormats.assign(std::begin(s_gBufferFormats), std::end(s_gBufferFormats));

	std::string error;
	m_gBufferPipeline = m_pipelineFactory->CreateRasterizationPipeline(gBufferProgram.get(), m_gBufferPass->GetRenderPass(), properties, error);

	if (m_gBufferPipeline == nullptr) {
		Logger::LogError("Failed to create the g buffer pipeline: " + error);
		return false;
	}

	if (m_gBufferPipeline->Initialize((UInt32)m_gBufferEntityGPUList.size()) == false) {
		Logger::LogError("Failed to create the descriptors of the g buffer pipeline");
		m_gBufferPipeline.reset();
		return false;
	}

	m_gBufferPipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraBuffer, 0, m_cameraStride);
	m_gBufferPass->AddPipeline(m_gBufferPipeline.get());

	for (auto& entityGPU : m_gBufferEntityGPUList) {
		auto mesh = entityGPU.gameEntity->GetRenderer()->GetMesh();

		if (mesh == nullptr)
			continue;

		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(mesh->GetGPUHandle());

		if (meshController == nullptr)
			continue;

		GameEntity* entity = entityGPU.gameEntity.get();

		m_gBufferPipeline->SetEntityDescriptor(entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffers[entityGPU.index].buffer);
		m_gBufferPipeline->SetEntityGeometry(entity, meshController->GetVertexBuffer(), meshController->GetIndexBuffer(), mesh->GetIndexCount());
	}

	return true;
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::CreateGBufferTargets()
{
	UInt32 width = m_swapChainCapability.currentExtent.width;
	UInt32 height = m_swapChainCapability.currentExtent.height;

	std::vector<VulkanRenderTexture*> targets;

	for (UInt32 i = 0; i < GBufferTargetCount; i++) {
		m_gBufferTargets[i] = m_bufferFactory->CreateRenderTexture(VulkanTextureDesc{
			.width = width,
			.height = height,
			.format = s_gBufferFormats[i],
			.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		});

		if (m_gBufferTargets[i] == nullptr)
			return false;

		targets.push_back(m_gBufferTargets[i].get());
	}

	VulkanRenderPassParams params;
	params.clearColors = { { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
	params.depthTarget = m_depthTexture.get();
	params.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	return m_gBufferPass->Initialize(m_logicDevice, targets, params);
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::DestroyGBufferTargets()
{
	for (auto& target : m_gBufferTargets)
		target.reset();
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::CreateSplinePipelines(const std::vector<VKEntityGPUData>& splineEntities)
{
	for (auto& entityGPU : splineEntities) {
		auto splineRenderer = std::static_pointer_cast<SplineRenderer>(entityGPU.gameEntity->GetRenderer());

		if (splineRenderer->GetSegments() <= 0)
			continue;

		if (splineRenderer->GetMaterial() == nullptr) {
			Logger::LogWarning("The spline renderer of " + entityGPU.gameEntity->GetName() + " has no material and is not drawn");
			continue;
		}

		SplineGPUData spline;
		spline.renderer = splineRenderer;
		spline.entity = entityGPU.gameEntity.get();
		spline.transformIndex = entityGPU.index;
		spline.vertexCount = (UInt32)splineRenderer->GetSegments() + 1;

		spline.vertexBuffer = m_bufferFactory->CreateStructuredBuffer<SplineVertex>(VulkanBufferDesc{
			.memoryType = VulkanMemoryType::GPUOnly,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		}, (int)spline.vertexCount);

		if (spline.vertexBuffer == nullptr) {
			Logger::LogError("Failed to create the vertex buffer of " + entityGPU.gameEntity->GetName());
			continue;
		}

		m_splines.push_back(std::move(spline));
	}

	if (m_splines.empty())
		return;

	auto computeProgram = GetInternalProgram("Bezier_Curve_Compute_Program");

	if (computeProgram == nullptr) {
		Logger::LogError("The spline compute program is not registered, the splines are not drawn");
		DestroySplineBuffers();
		return;
	}

	std::string error;
	m_splineComputeModule = m_pipelineFactory->CreateComputePipeline(computeProgram.get(), error);

	if (m_splineComputeModule == nullptr) {
		Logger::LogError("Failed to create the spline compute pipeline: " + error);
		DestroySplineBuffers();
		return;
	}

	if (m_splineComputeModule->Initialize((UInt32)m_splines.size()) == false) {
		Logger::LogError("Failed to create the descriptors of the spline compute pipeline");
		m_splineComputeModule.reset();
		DestroySplineBuffers();
		return;
	}

	UInt32 threadsPerGroup = std::max<UInt32>(1, m_splineComputeModule->GetProgram()->GetThreadGroupReflection().xThread);
	std::map<ref<Material>, std::vector<size_t>> materialMap;

	for (size_t i = 0; i < m_splines.size(); i++) {
		auto& spline = m_splines[i];

		m_splineComputeModule->SetEntityDescriptor(spline.entity, INTERNAL_VERTEX_BUFFER_NAME, spline.vertexBuffer->GetBuffer());
		m_splineComputeModule->SetEntityThreadGroupCount(spline.entity, (spline.vertexCount + threadsPerGroup - 1) / threadsPerGroup);
		spline.renderer->SetDirty();

		materialMap[spline.renderer->GetMaterial()].push_back(i);
	}

	Rasterization::RasterizationPipelineProperties properties;
	properties.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	properties.cullMode = VK_CULL_MODE_NONE;

	for (auto& [material, splineIndices] : materialMap) {
		auto pipeline = m_pipelineFactory->CreateRasterizationPipeline(material.get(), m_mainPass->GetRenderPass(), properties, error);

		if (pipeline == nullptr) {
			Logger::LogError("Failed to create the spline rasterization pipeline: " + error);
			continue;
		}

		if (pipeline->Initialize((UInt32)splineIndices.size()) == false) {
			Logger::LogError("Failed to create the descriptors of the spline rasterization pipeline");
			continue;
		}

		pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraBuffer, 0, m_cameraStride);
		pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightBuffer, 0, m_lightStride);

		for (auto splineIndex : splineIndices) {
			auto& spline = m_splines[splineIndex];

			pipeline->SetEntityDescriptor(spline.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffers[spline.transformIndex].buffer);
			pipeline->SetEntityGeometry(spline.entity, spline.vertexBuffer->GetBuffer(), spline.vertexCount);
			spline.rasterModule = pipeline;
		}

		m_rasterizationModules.push_back(pipeline);
	}
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::UpdateSplines()
{
	for (auto& spline : m_splines) {
		if (spline.renderer->IsDirty() == false)
			continue;

		auto& curve = spline.renderer->GetCurve();
		float tileFactor = spline.renderer->GetTileFactor();
		float length = curve.InterpolateLength(1.0f);

		m_splineComputeModule->SetEntityConstant(spline.entity, "_startPoint", curve.m_point1);
		m_splineComputeModule->SetEntityConstant(spline.entity, "_midPoint", curve.m_point2);
		m_splineComputeModule->SetEntityConstant(spline.entity, "_endPoint", curve.m_point3);
		m_splineComputeModule->SetEntityConstant(spline.entity, "_tileFactor", tileFactor);
		m_splineComputeModule->SetEntityConstant(spline.entity, "_length", length);

		if (spline.rasterModule != nullptr) {
			float width = spline.renderer->GetWidth();
			spline.rasterModule->SetEntityConstant(spline.entity, INTERNAL_SPLINE_WIDTH_NAME, width);
		}
	}
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::DestroySplineBuffers()
{
	m_splines.clear();
}

ref<LuxonEngine::Rendering::ShaderProgram> LuxonEngine::Rendering::Vulkan::VulkanHybridContext::GetInternalProgram(const std::string& identifier) const
{
	if (m_shaderProgramRegistery == nullptr)
		return nullptr;

	ShaderProgram* program = m_shaderProgramRegistery->GetShaderProgram(identifier);
	if (program == nullptr)
		return nullptr;

	// the registery owns the program. the empty deleter keeps the ref from deleting it
	return ref<ShaderProgram>(program, [](ShaderProgram*) {});
}

std::vector<ref<LuxonEngine::Rendering::Vulkan::MeshShading::VulkanMeshPipelineModule>> LuxonEngine::Rendering::Vulkan::VulkanHybridContext::CreateSpikeMeshPipelines()
{
	std::vector<ref<MeshShading::VulkanMeshPipelineModule>> pipelines;

	if (m_pipelineFactory == nullptr)
		return pipelines;

	struct SpikeData {
		GameEntity* entity;
		UInt32 index; // index of the entity in the transform buffer
		ref<VulkanMeshController> meshController;
		ref<SpikeMeshRenderer> spikeRenderer;
	};

	// group the spike entities by material, one mesh pipeline is created per material
	std::map<ref<Material>, std::vector<SpikeData>> spikeMap;

	for (auto& entityGPU : m_entityGPUList) {
		auto spikeRenderer = std::dynamic_pointer_cast<SpikeMeshRenderer>(entityGPU.gameEntity->GetRenderer());
		if (spikeRenderer == nullptr || spikeRenderer->GetMaterial() == nullptr || spikeRenderer->GetMesh() == nullptr)
			continue;

		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(spikeRenderer->GetMesh()->GetGPUHandle());
		if (meshController == nullptr) // the mesh is not uploaded to the GPU
			continue;

		spikeMap[spikeRenderer->GetMaterial()].push_back(SpikeData{
			.entity = entityGPU.gameEntity.get(),
			.index = entityGPU.index,
			.meshController = meshController,
			.spikeRenderer = spikeRenderer,
			});
	}

	// the pyramid shader handles 32 mesh groups of 32 triangles in each task group
	constexpr UInt32 trianglesPerTaskGroup = 32 * 32;

	std::string error;
	MeshShading::MeshPipelineProperties properties;

	for (auto& [material, spikeList] : spikeMap) {
		auto spikeMeshPipeline = m_pipelineFactory->CreateMeshPipeline(material.get(), m_mainPass->GetRenderPass(), properties, error);

		if (spikeMeshPipeline == nullptr) {
			Logger::LogError("Failed to create the spike mesh pipeline: " + error);
			continue;
		}

		if (spikeMeshPipeline->Initialize((UInt32)spikeList.size()) == false) {
			Logger::LogError("Failed to create the descriptors of the spike mesh pipeline");
			continue;
		}

		spikeMeshPipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraBuffer, 0, m_cameraStride);
		spikeMeshPipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightBuffer, 0, m_lightStride);

		for (auto& spikeData : spikeList) {
			// every entity points to its own range of the shared transform buffer, no dynamic offset is needed
			spikeMeshPipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffers[spikeData.index].buffer);

			// the mesh buffers are not created with the storage usage, so storage buffers sharing their memory are bound instead
			VkBuffer vertexStorageBuffer = spikeData.meshController->CreateVertexStorageBuffer();
			VkBuffer indexStorageBuffer = spikeData.meshController->CreateIndexStorageBuffer();
			m_meshStorageBuffers.push_back(vertexStorageBuffer);
			m_meshStorageBuffers.push_back(indexStorageBuffer);

			spikeMeshPipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_VERTEX_BUFFER_NAME, vertexStorageBuffer);
			spikeMeshPipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_INDEX_BUFFER_NAME, indexStorageBuffer);
			spikeMeshPipeline->SetEntityConstant(spikeData.entity, "_height", spikeData.spikeRenderer->GetSpikeHeight());

			// every entity launches only the task groups its own mesh needs
			UInt32 triangleCount = spikeData.spikeRenderer->GetMesh()->GetIndexCount() / 3;
			spikeMeshPipeline->SetEntityThreadGroupCount(spikeData.entity, (triangleCount + trianglesPerTaskGroup - 1) / trianglesPerTaskGroup);
		}

		pipelines.push_back(spikeMeshPipeline);
	}

	return pipelines;
}

std::vector<ref<LuxonEngine::Rendering::Vulkan::MeshShading::VulkanMeshPipelineModule>> LuxonEngine::Rendering::Vulkan::VulkanHybridContext::CreateSurfaceInstancePipelines()
{
	std::vector<ref<MeshShading::VulkanMeshPipelineModule>> pipelines;

	if (m_pipelineFactory == nullptr)
		return pipelines;

	struct SurfaceInstanceData {
		GameEntity* entity;
		UInt32 index; // index of the entity in the transform buffer
		ref<VulkanTexture2DController> maskTexture;
		ref<SurfaceInstanceRenderer> surfaceInstanceRenderer;
	};

	std::map<ref<Material>, std::vector<SurfaceInstanceData>> surfaceInstanceMap;
	std::vector<ref<Mesh>> instanceMeshes;

	// group the surface instance entities by material, one mesh pipeline is created per material
	for (auto& entityGPU : m_entityGPUList) {
		auto surfaceInstanceRenderer = std::dynamic_pointer_cast<SurfaceInstanceRenderer>(entityGPU.gameEntity->GetRenderer());
		if (surfaceInstanceRenderer == nullptr || surfaceInstanceRenderer->GetMaterial() == nullptr || surfaceInstanceRenderer->GetNearMesh() == nullptr ||
			surfaceInstanceRenderer->GetMaskTexture() == nullptr || surfaceInstanceRenderer->GetDensity() <= 0.0f)
			continue;

		// outside the editor the textures are not uploaded on import. already uploaded textures are skipped
		m_assetManager->UploadTextureToGPU(surfaceInstanceRenderer->GetMaskTexture());

		auto maskTexture = std::dynamic_pointer_cast<VulkanTexture2DController>(surfaceInstanceRenderer->GetMaskTexture()->GetGPUHandle());
		if (maskTexture == nullptr) // the mask is not uploaded to the GPU
			continue;

		instanceMeshes.push_back(surfaceInstanceRenderer->GetNearMesh());

		if (surfaceInstanceRenderer->GetFarMesh() != nullptr)
			instanceMeshes.push_back(surfaceInstanceRenderer->GetFarMesh());

		surfaceInstanceMap[surfaceInstanceRenderer->GetMaterial()].push_back(SurfaceInstanceData{
			.entity = entityGPU.gameEntity.get(),
			.index = entityGPU.index,
			.maskTexture = maskTexture,
			.surfaceInstanceRenderer = surfaceInstanceRenderer,
			});
	}

	// the base mesh of the renderer is null, so these meshes are not uploaded with the other meshes. already uploaded meshes are skipped
	if (instanceMeshes.empty() == false)
		m_assetManager->UploadMeshesToGPU(instanceMeshes);

	// one task group per cell, the task group count of every dimension is limited to 65535 groups
	constexpr UInt32 maxGroupsPerDimension = 65535;

	std::string error;
	MeshShading::MeshPipelineProperties properties;

	for (auto& [material, surfaceInstanceList] : surfaceInstanceMap) {
		auto surfaceInstancePipeline = m_pipelineFactory->CreateMeshPipeline(material.get(), m_mainPass->GetRenderPass(), properties, error);

		if (surfaceInstancePipeline == nullptr) {
			Logger::LogError("Failed to create the surface instance pipeline: " + error);
			continue;
		}

		if (surfaceInstancePipeline->Initialize((UInt32)surfaceInstanceList.size()) == false) {
			Logger::LogError("Failed to create the descriptors of the surface instance pipeline");
			continue;
		}

		surfaceInstancePipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraBuffer, 0, m_cameraStride);
		surfaceInstancePipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightBuffer, 0, m_lightStride);

		for (auto& surfaceInstanceData : surfaceInstanceList) {
			auto& renderer = surfaceInstanceData.surfaceInstanceRenderer;

			// without a far mesh, the near mesh is bound to both levels
			auto farMesh = renderer->GetFarMesh() != nullptr ? renderer->GetFarMesh() : renderer->GetNearMesh();
			auto nearMeshController = std::dynamic_pointer_cast<VulkanMeshController>(renderer->GetNearMesh()->GetGPUHandle());
			auto farMeshController = std::dynamic_pointer_cast<VulkanMeshController>(farMesh->GetGPUHandle());

			if (nearMeshController == nullptr || farMeshController == nullptr) // the meshes are not uploaded to the GPU, the entity is not drawn
				continue;

			// the mesh buffers are not created with the storage usage, so storage buffers sharing their memory are bound instead
			VkBuffer nearVertexBuffer = nearMeshController->CreateVertexStorageBuffer();
			VkBuffer nearIndexBuffer = nearMeshController->CreateIndexStorageBuffer();
			VkBuffer farVertexBuffer = farMeshController->CreateVertexStorageBuffer();
			VkBuffer farIndexBuffer = farMeshController->CreateIndexStorageBuffer();
			m_meshStorageBuffers.insert(m_meshStorageBuffers.end(), { nearVertexBuffer, nearIndexBuffer, farVertexBuffer, farIndexBuffer });

			// every entity points to its own range of the shared transform buffer, no dynamic offset is needed
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffers[surfaceInstanceData.index].buffer);
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_MASK_TEXTURE_NAME, surfaceInstanceData.maskTexture->GetImageView());
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_NEAR_VERTEX_BUFFER_NAME, nearVertexBuffer);
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_NEAR_INDEX_BUFFER_NAME, nearIndexBuffer);
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_FAR_VERTEX_BUFFER_NAME, farVertexBuffer);
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_FAR_INDEX_BUFFER_NAME, farIndexBuffer);

			// the plane size comes from the x and z scale of the transform, density is the number of cells per unit length
			Vector3 planeSize = surfaceInstanceData.entity->GetTransform()->Scale();
			float density = renderer->GetDensity();
			UInt32 cellCount[2] = {
				(std::max)(1u, (UInt32)std::ceil(std::abs(planeSize.x) * density)),
				(std::max)(1u, (UInt32)std::ceil(std::abs(planeSize.z) * density)),
			};

			if (cellCount[0] > maxGroupsPerDimension || cellCount[1] > maxGroupsPerDimension) {
				Logger::LogWarning("The surface instance grid of " + surfaceInstanceData.entity->GetName() + " is clamped to 65535 cells per axis");
				cellCount[0] = (std::min)(cellCount[0], maxGroupsPerDimension);
				cellCount[1] = (std::min)(cellCount[1], maxGroupsPerDimension);
			}

			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_step", 1.0f / density);
			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_totalIndices", cellCount);
			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_entityScale", renderer->GetInstanceScale());
			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_distance", renderer->GetNearDistance());
			surfaceInstancePipeline->SetEntityThreadGroupCount(surfaceInstanceData.entity, cellCount[0], cellCount[1]);
		}

		pipelines.push_back(surfaceInstancePipeline);
	}

	return pipelines;
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::DestroyMeshStorageBuffers()
{
	// the buffers only share the memory of the mesh buffers, the memory is released with the mesh
	for (auto& storageBuffer : m_meshStorageBuffers)
		vkDestroyBuffer(m_logicDevice, storageBuffer, nullptr);

	m_meshStorageBuffers.clear();
}
