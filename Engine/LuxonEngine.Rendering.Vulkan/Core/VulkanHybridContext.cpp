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
#include "Rasterization/VulkanRasterizationMaterial.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "VulkanSplinePipelineModule.h"
#include "Compute/SPIRVComputeProgram.h"
#include "VulkanGBufferPipelineModule.h"
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
	// the mesh pipelines are destroyed before the storage buffers they use
	m_meshShadingPipelines.clear();
	DestroyMeshStorageBuffers();

	vkDestroyBuffer(m_logicDevice, m_transformBuffer, nullptr);
	vkFreeMemory(m_logicDevice, m_transformBufferMemory, nullptr);

	vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
	vkFreeMemory(m_logicDevice, m_depthMemory, nullptr);

	for (auto framebuffer : m_swapChainFramebuffers) {
		vkDestroyFramebuffer(m_logicDevice, framebuffer, nullptr);
	}

	vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);
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

	VkImageCreateInfo imgInfo{};
	imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imgInfo.extent = { m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height, 1 };
	imgInfo.mipLevels = 1;
	imgInfo.arrayLayers = 1; 
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	m_bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_rtOutputImage, &m_rtOutputImageMemory);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_rtOutputImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imgInfo.format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(m_logicDevice, &viewInfo, nullptr, &m_rtOutputImageView);

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
	m_clearValues[0].color = { colorArray[0], colorArray[1], colorArray[2], colorArray[3] };
	m_clearValues[1].depthStencil = { 1.0f, 0 };
	
	// Create Uniform Buffers for Transforms
	m_bufferFactory->CreateBuffer(sizeof(TransformGPU), scene->entities.size()
		, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_transformBuffer, &m_transformBufferMemory, &m_transformStride);


	std::map<ref<Material>, ref<Rasterization::VulkanRasterizationMaterial>> usedMaterials;
	UInt32 index = 0;
	m_entityGPUList.reserve(scene->entities.size());


	for (auto& entity : scene->entities) {
		m_entityGPUList.push_back({ entity, index });
		index++;

		// spike and surface instance entities use mesh programs, they are drawn by the mesh pipelines instead of a rasterization material
		if (std::dynamic_pointer_cast<SpikeMeshRenderer>(entity->GetRenderer()) != nullptr ||
			std::dynamic_pointer_cast<SurfaceInstanceRenderer>(entity->GetRenderer()) != nullptr)
			continue;

		auto material = entity->GetRenderer()->GetMaterial();
		auto matIT = usedMaterials.find(material);

		if (matIT == usedMaterials.end()) {
			usedMaterials.emplace(material, std::make_shared<Rasterization::VulkanRasterizationMaterial>(material, m_logicDevice)).first;
		}
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(20);
	UInt32 setcount = 0;

	for (auto& matPair : usedMaterials) {
		auto program = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>(matPair.first->GetProgram());
		auto& reflection = program->GetReflection();
		setcount += reflection.GetDescriptorLayoutCount();
		auto& descriptors = reflection.GetDescriptors();

		for (auto& descriptor : descriptors) {
			auto it = std::find_if(poolSizes.begin(), poolSizes.end(), [descriptor](const VkDescriptorPoolSize& poolSize) {
				return descriptor.descriptorType == poolSize.type;
				});

			if (it != poolSizes.end())
				(*it).descriptorCount++;
			else
				poolSizes.push_back(VkDescriptorPoolSize{
				.type = descriptor.descriptorType,
				.descriptorCount = 1,
					});
		}
	}

	setcount += m_entityGPUList.size();

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = (UInt32)usedMaterials.size(),
		});

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = (UInt32)m_entityGPUList.size(),
		});

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = setcount,
		.poolSizeCount = (UInt32)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(m_logicDevice, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return false;

	for (auto& entityGPU : m_entityGPUList) {
		auto& entity = entityGPU.gameEntity;
		auto Renderer = entity->GetRenderer();

		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(Renderer);

		if (meshRenderer != nullptr) {
			auto& gpuMateiral = usedMaterials[meshRenderer->GetMaterial()];
			ref<Rasterization::VulkanRasterizationPipelineModule> rasterizationModule = std::make_shared<Rasterization::VulkanRasterizationPipelineModule>(m_logicDevice);
			if (rasterizationModule->Initialize(meshRenderer->GetMesh(), gpuMateiral, m_renderPass)) {
				m_rasterizationModules.push_back(rasterizationModule);
				rasterizationModule->SetDescriptorOffset(HLSL_OBJECT_TRANSFORM_DATA_NAME, entityGPU.index * m_transformStride);
				rasterizationModule->SetDescriptorOffset(HLSL_CAMERA_DATA_NAME, 0);
				rasterizationModule->SetDescriptorOffset(HLSL_LIGHT_DATA_NAME, 0);
			}

			continue;
		}

		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(Renderer);

		if (splineRenderer != nullptr) {
			auto& gpuMateiral = usedMaterials[splineRenderer->GetMaterial()];
			ref<VulkanSplinePipelineModule> splineModule = std::make_shared<VulkanSplinePipelineModule>(m_logicDevice);
			SplineEntityData splineEntityData{
				.splineRenderer = splineRenderer,
				.material = gpuMateiral,
				.computeProgram = std::dynamic_pointer_cast<Compute::SPIRVComputeProgram>(GetInternalProgram("Bezier_Curve_Compute_Program")),
			};
			if (splineModule->Initialize(splineEntityData, m_renderPass, m_descriptorPool)) {
				m_splineModues.push_back(splineModule);
				splineModule->WriteOffset(HLSL_OBJECT_TRANSFORM_DATA_NAME, entityGPU.index * m_transformStride);
				splineModule->WriteOffset(HLSL_CAMERA_DATA_NAME, 0);
				splineModule->WriteOffset(HLSL_LIGHT_DATA_NAME, 0);
				splineRenderer->SetDirty();
			}

			continue;
		}

		auto gBufferRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(Renderer);

		if (gBufferRenderer != nullptr) {
			m_gBufferEntityGPUList.push_back(entityGPU);

			auto& gpuMateiral = usedMaterials[gBufferRenderer->GetMaterial()];
			ref<Rasterization::VulkanRasterizationPipelineModule> rasterizationModule = std::make_shared<Rasterization::VulkanRasterizationPipelineModule>(m_logicDevice);
			if (rasterizationModule->Initialize(gBufferRenderer->GetMesh(), gpuMateiral, m_renderPass)) {
				m_gBufferRasterizationModules.push_back(rasterizationModule);
				rasterizationModule->SetDescriptorOffset(HLSL_OBJECT_TRANSFORM_DATA_NAME, entityGPU.index * m_transformStride);
				rasterizationModule->SetDescriptorOffset(HLSL_CAMERA_DATA_NAME, 0);
				rasterizationModule->SetDescriptorOffset(HLSL_LIGHT_DATA_NAME, 0);
			}
		}
	}

	for (auto& matPair : usedMaterials) {
		if (matPair.second->Initialize(m_descriptorPool) == false)
			continue;
		matPair.second->WriteBuffer(HLSL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffer, m_transformStride);
		matPair.second->WriteBuffer(HLSL_CAMERA_DATA_NAME, m_cameraBuffer, m_cameraStride);
		matPair.second->WriteBuffer(HLSL_LIGHT_DATA_NAME, m_lightBuffer, m_lightStride);
	}

	m_meshShadingPipelines = CreateSpikeMeshPipelines();

	auto surfaceInstancePipelines = CreateSurfaceInstancePipelines();
	m_meshShadingPipelines.insert(m_meshShadingPipelines.end(), surfaceInstancePipelines.begin(), surfaceInstancePipelines.end());

	if (m_gBufferEntityGPUList.size() > 0) {
		auto gBufferProgram = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>( GetInternalProgram("G_Buffer_Program"));
		m_gbufferModule = std::make_shared<VulkanGBufferPipelineModule>();
		m_gbufferModule->InitializePipeline(m_gBufferEntityGPUList, gBufferProgram, m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height, m_depthImageView);
		m_gbufferModule->WriteBuffer(HLSL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffer, m_transformStride);
		m_gbufferModule->WriteBuffer(HLSL_CAMERA_DATA_NAME, m_cameraBuffer, m_cameraStride);

		auto gBufferGlobalProgram = GetInternalProgram("G_Buffer_RT_Global_Program");

		auto materialFactory = VulkanDeviceManager::Instance()->CreateMaterialFactory();
		auto gBufferGlobalMaterial = materialFactory->CreateMaterial(gBufferGlobalProgram);
		gBufferGlobalMaterial->SetValue("missColor", &m_clearValues[0], 4 * sizeof(Float));
		m_rayTracingModule = std::make_shared<RayTracing::VulkanRayTracingPipelineModule>();
		if (m_rayTracingModule->Initialize(scene->entities, gBufferGlobalMaterial, m_cameraBuffer, m_lightBuffer, m_transformBuffer, m_swapChainCapability.currentExtent) == false)
			return false;

		m_rayTracingModule->SetImage("_PositionTexture", m_gbufferModule->GetPositionImageView());
		m_rayTracingModule->SetImage("_NormalTexture", m_gbufferModule->GetNormalImageView());
		m_rayTracingModule->SetImage("_MaskTexture", m_gbufferModule->GetMaskImageView());
		m_rayTracingModule->SetImage(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputImageView);

		for(auto& [material, rasterMaterial] : usedMaterials) {
			rasterMaterial->SetImageView("_PositionTexture", m_gbufferModule->GetPositionImageView());
			rasterMaterial->SetImageView("_NormalTexture", m_gbufferModule->GetNormalImageView());
			rasterMaterial->SetImageView("_MaskTexture", m_gbufferModule->GetMaskImageView());
			rasterMaterial->SetImageView(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputImageView);
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

	for (auto& m_splineModues : m_splineModues) {
		m_splineModues->ComputeCommand(m_commandBuffer);
	}

	if (m_gbufferModule != nullptr) {
		m_gbufferModule->RenderCommand(m_commandBuffer);

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
		gBufferBarriers[0].image = m_gbufferModule->GetPositionImage();
		gBufferBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		gBufferBarriers[0].subresourceRange.baseMipLevel = 0;
		gBufferBarriers[0].subresourceRange.levelCount = 1;
		gBufferBarriers[0].subresourceRange.baseArrayLayer = 0;
		gBufferBarriers[0].subresourceRange.layerCount = 1;

		// Normal image
		gBufferBarriers[1] = gBufferBarriers[0];
		gBufferBarriers[1].image = m_gbufferModule->GetNormalImage();

		// Mask image
		gBufferBarriers[2] = gBufferBarriers[0];
		gBufferBarriers[2].image = m_gbufferModule->GetMaskImage();

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
		rtOutImageBarrier.image = m_rtOutputImage;
		rtOutImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		rtOutImageBarrier.subresourceRange.baseMipLevel = 0;
		rtOutImageBarrier.subresourceRange.levelCount = 1;
		rtOutImageBarrier.subresourceRange.baseArrayLayer = 0;
		rtOutImageBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			0, 0, nullptr, 0, nullptr, 1, &rtOutImageBarrier);


		m_rayTracingModule->UpdateTLAS(m_commandBuffer);
		m_rayTracingModule->RenderCommand(m_commandBuffer);

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

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_renderPass,
		.framebuffer = m_swapChainFramebuffers[imageIndex],
		.renderArea = {
			.offset = { 0, 0 },
			.extent = m_swapChainCapability.currentExtent,
		},
		.clearValueCount = 2,
		.pClearValues = m_clearValues,
	};

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainCapability.currentExtent.width;
	viewport.height = (float)m_swapChainCapability.currentExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainCapability.currentExtent;

	vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

	// Render Objects here
	for (auto& module : m_rasterizationModules) {
		module->RenderCommand(m_commandBuffer);
	}

	for (auto& module : m_splineModues) {
		module->RenderCommand(m_commandBuffer);
	}

	for(auto& module : m_gBufferRasterizationModules) {
		module->RenderCommand(m_commandBuffer);
	}

	for (auto& meshShadingPipeline : m_meshShadingPipelines)
		meshShadingPipeline->Dispatch(m_commandBuffer);

	vkCmdEndRenderPass(m_commandBuffer);
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
	if (m_depthImage != VK_NULL_HANDLE) {
		vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
		vkFreeMemory(m_logicDevice, m_depthMemory, nullptr);
	}

	for (auto framebuffer : m_swapChainFramebuffers)
		vkDestroyFramebuffer(m_logicDevice, framebuffer, nullptr);

	RecreateSwapChain(m_swapChainUsageFlags);

	InitializeDepthBuffer();

	m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
	for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
		VkImageView fbAttachments[2] = { m_swapChainImageViews[i], m_depthImageView };

		VkFramebufferCreateInfo fbInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_renderPass,
			.attachmentCount = 2,
			.pAttachments = fbAttachments,
			.width = m_swapChainCapability.currentExtent.width,
			.height = m_swapChainCapability.currentExtent.height,
			.layers = 1,
		};

		vkCreateFramebuffer(m_logicDevice, &fbInfo, nullptr, &m_swapChainFramebuffers[i]);
	}

	if (m_gbufferModule != nullptr) {
		m_gbufferModule->Resize(m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height, m_depthImageView);
	}

	if (m_rayTracingModule != nullptr) {
		vkDestroyImageView(m_logicDevice, m_rtOutputImageView, nullptr);
		vkDestroyImage(m_logicDevice, m_rtOutputImage, nullptr);
		vkFreeMemory(m_logicDevice, m_rtOutputImageMemory, nullptr);

		VkImageCreateInfo imgInfo{};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		imgInfo.extent = { m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height, 1 };
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		m_bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_rtOutputImage, &m_rtOutputImageMemory);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_rtOutputImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = imgInfo.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(m_logicDevice, &viewInfo, nullptr, &m_rtOutputImageView);

		m_rayTracingModule->SetExtent(m_swapChainCapability.currentExtent);
		m_rayTracingModule->SetImage(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputImageView);
	
		if (m_gbufferModule != nullptr) {
			m_rayTracingModule->SetImage("_PositionTexture", m_gbufferModule->GetPositionImageView());
			m_rayTracingModule->SetImage("_NormalTexture", m_gbufferModule->GetNormalImageView());
			m_rayTracingModule->SetImage("_MaskTexture", m_gbufferModule->GetMaskImageView());
		
		
			for (auto& rasterModule : m_gBufferRasterizationModules) {
				auto rasterMaterial = rasterModule->GetMaterial();
				rasterMaterial->SetImageView("_PositionTexture", m_gbufferModule->GetPositionImageView());
				rasterMaterial->SetImageView("_NormalTexture", m_gbufferModule->GetNormalImageView());
				rasterMaterial->SetImageView("_MaskTexture", m_gbufferModule->GetMaskImageView());
				rasterMaterial->SetImageView(HLSL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputImageView);
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
	// Create depth image
	VkImageCreateInfo imgInfo{
	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	.imageType = VK_IMAGE_TYPE_2D,
	.format = m_depthFormat,
	.extent = { m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height, 1 },
	.mipLevels = 1,
	.arrayLayers = 1,
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.tiling = VK_IMAGE_TILING_OPTIMAL,
	.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	m_bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_depthImage, &m_depthMemory);

	// Create depth image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(m_logicDevice, &viewInfo, nullptr, &m_depthImageView);

	return true;
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::InitializeRenderPass()
{
	VkAttachmentDescription colorAttachment{
		.format = m_swapChainFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentDescription depthAttachment{
		.format = m_depthFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription colorSubpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	VkRenderPassCreateInfo rpInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &colorSubpass,
	};

	auto result = vkCreateRenderPass(m_logicDevice, &rpInfo, nullptr, &m_renderPass);
	if (result != VK_SUCCESS)
		return false;

	// Create Framebuffers for Swap chain images
	m_swapChainFramebuffers.resize(2);
	VkFramebuffer* framebuffer = m_swapChainFramebuffers.data();

	for (auto view : m_swapChainImageViews) {
		VkImageView imageViews[2] = { view, m_depthImageView };

		VkFramebufferCreateInfo fbInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_renderPass,
			.attachmentCount = 2,
			.pAttachments = imageViews,
			.width = m_swapChainCapability.currentExtent.width,
			.height = m_swapChainCapability.currentExtent.height,
			.layers = 1,
		};
		vkCreateFramebuffer(m_logicDevice, &fbInfo, nullptr, framebuffer);
		framebuffer++;
	}
	return true;
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::UpdateEntityTransforms()
{
	void* data;
	vkMapMemory(m_logicDevice, m_transformBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);

	for (auto& entityGPU : m_entityGPUList) {
		m_transformData.modelMatrix = entityGPU.gameEntity->GetTransform()->Matrix();
		m_transformData.modelViewMatrix = m_cameraGPU.viewMatrix * m_transformData.modelMatrix;
		m_transformData.rotationMatrix = entityGPU.gameEntity->GetTransform()->RotateMatrix();

		std::memcpy((Byte*)data + entityGPU.index * m_transformStride, &m_transformData, sizeof(TransformGPU));
	}

	vkUnmapMemory(m_logicDevice, m_transformBufferMemory);
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
		auto spikeMeshPipeline = m_pipelineFactory->CreateMeshPipeline(material.get(), m_renderPass, properties, error);

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
			spikeMeshPipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffer,
				(VkDeviceSize)spikeData.index * m_transformStride, m_transformStride);

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
		auto surfaceInstancePipeline = m_pipelineFactory->CreateMeshPipeline(material.get(), m_renderPass, properties, error);

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
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffer,
				(VkDeviceSize)surfaceInstanceData.index * m_transformStride, m_transformStride);
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
