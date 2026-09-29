#include "vulkan-pch.h"
#include "VulkanEditorGraphicContext.h"
#include "Core/Scene.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "Core/Transform.h"
#include "Rendering/Renderer.h"
#include "Rendering/MeshRenderer.h"
#include "VulkanAssetManager.h"
#include "VulkanUtilities.h"
#include "VulkanBufferFactory.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Rasterization/VulkanRasterizationMaterial.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "Core/VulkanMaterialFactory.h"
#include "Core/VulkanPipelineFactory.h"
#include "Mesh/VulkanMeshPipelineModule.h"
#include "Core/VulkanMeshController.h"
#include "Rendering/ShaderInternalNames.h"
#include "Rendering/SpikeMeshRenderer.h"
#include "Core/Logger.h"
#include <map>
#include <set>
#include <algorithm>
#include <Rendering/RayTracingComponent.h>
#include <Rendering/GBufferRTReflectionRenderer.h>

LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::VulkanEditorGraphicContext(
	const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window,
	const ref<VulkanPipelineFactory>& pipelineFactory)
	: VulkanGraphicContext(vkInstance, surfaceQueueFamilyIndex, window)
	, m_pipelineFactory(pipelineFactory)
	, m_transformStride(0)
	, m_transformBuffer(VK_NULL_HANDLE)
	, m_transformBufferMemory(VK_NULL_HANDLE)
	, m_renderPass(VK_NULL_HANDLE)
	, m_depthImage(VK_NULL_HANDLE)
	, m_depthMemory(VK_NULL_HANDLE)
	, m_depthImageView(VK_NULL_HANDLE)
{
	m_swapChainUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::~VulkanEditorGraphicContext()
{
	if (m_transformBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(m_logicDevice, m_transformBuffer, nullptr);
		vkFreeMemory(m_logicDevice, m_transformBufferMemory, nullptr);
	}

	if (m_depthImage != VK_NULL_HANDLE) {
		vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
		vkFreeMemory(m_logicDevice, m_depthMemory, nullptr);
	}

	for (auto framebuffer : m_swapChainFramebuffers)
		vkDestroyFramebuffer(m_logicDevice, framebuffer, nullptr);

	if (m_renderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);

	if (m_descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);

	m_meshShadingPipelines.clear();
	DestroyMeshStorageBuffers();
}

bool LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::Initialize()
{
	if(InitializeSurface() == false)
		return false;

	if (InitializeSwapChain(m_swapChainUsageFlags) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	if (InitializeRenderPass() == false)
		return false;

	if (InitializeCommandObjects() == false)
		return false;

	if (InitializeFencesAndSemaphores() == false)
		return false;

	return true;
}

bool LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::PrepareScene(const ref<Scene>& scene)
{
	m_scene = scene;

	UploadMeshesToGPU(scene->entities);

	if (InitializeCameraBuffer(scene->mainCamera) == false)
		return false;

	SceneLightData editorLightData;
	editorLightData.directionalLights.push_back(DirectionalLight{
		.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
		.direction = Vector3(2.0f, -6.0f, 2.0f),
		.intensity = 0.5f,
		});

	if (InitializeLightBuffer(editorLightData) == false)
		return false;

	m_bufferFactory->CreateBuffer(
		sizeof(TransformGPU),
		scene->entities.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&m_transformBuffer, &m_transformBufferMemory, &m_transformStride);

	UInt32 index = 0;
	m_entityGPUList.reserve(scene->entities.size());
	for (auto& entity : scene->entities) {
		m_entityGPUList.push_back({ entity, index });
		index++;
	}

	m_overrideMaterial = scene->rtGlobalMaterial;
	InitializePipelines(m_overrideMaterial);

	return true;
}

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::Render()
{
	if (m_scene)
		SyncEntities(m_scene);

	UpdateCameraBuffer();
	UpdateEntityTransforms();

	vkResetFences(m_logicDevice, 1, &m_fence);

	UInt32 imageIndex;
	vkAcquireNextImageKHR(m_logicDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(m_commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr,
	};

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.2f, 0.4f, 0.6f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

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
		.pClearValues = clearValues,
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

	for (auto& module : m_rasterizationModules)
		module->RenderCommand(m_commandBuffer);

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

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::Resize(UInt32 width, UInt32 height)
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

}

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::UploadMeshesToGPU(const std::vector<ref<GameEntity>>& entities)
{
	std::set<ref<Mesh>> uniqueMeshes;
	for (auto& entity : entities) {
		auto renderer = entity->GetRenderer();
		if (renderer != nullptr) {
			auto mesh = renderer->GetMesh();

			if (mesh != nullptr)
				uniqueMeshes.insert(mesh);
		}

		auto rtcomponent = entity->GetRayTracingComponent();

		if (rtcomponent != nullptr)
			uniqueMeshes.insert(rtcomponent->GetMesh());
	}
	m_assetManager->UploadMeshesToGPU(std::vector<ref<Mesh>>(uniqueMeshes.begin(), uniqueMeshes.end()));
}

bool LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::InitializeDepthBuffer()
{
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

bool LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::InitializeRenderPass()
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

	if (vkCreateRenderPass(m_logicDevice, &rpInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		return false;

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

	return true;
}

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::UpdateEntityTransforms()
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

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::InitializePipelines(const ref<Material>& overrideMaterial)
{
	m_rasterizationModules.clear();
	m_sharedRasterMaterial.reset();
	m_meshShadingPipelines.clear();
	DestroyMeshStorageBuffers();

	if (m_descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);
		m_descriptorPool = VK_NULL_HANDLE;
	}

	// spike entities use their own material, so they do not depend on the override material
	m_meshShadingPipelines = CreateSpikeMeshPipelines();

	if (m_entityGPUList.empty() || overrideMaterial == nullptr)
		return;

	// Build pool sizes from the override material's shader reflection
	auto program = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>(overrideMaterial->GetProgram());
	auto& reflection = program->GetReflection();
	auto& descriptors = reflection.GetDescriptors();

	std::vector<VkDescriptorPoolSize> poolSizes;
	UInt32 setCount = reflection.GetDescriptorLayoutCount();

	for (auto& descriptor : descriptors) {
		auto it = std::find_if(poolSizes.begin(), poolSizes.end(), [&descriptor](const VkDescriptorPoolSize& ps) {
			return ps.type == descriptor.descriptorType;
			});

		if (it != poolSizes.end())
			it->descriptorCount++;
		else
			poolSizes.push_back(VkDescriptorPoolSize{ .type = descriptor.descriptorType, .descriptorCount = 1 });
	}

	// One storage buffer slot per entity for transforms
	setCount += static_cast<UInt32>(m_entityGPUList.size());
	poolSizes.push_back(VkDescriptorPoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = static_cast<UInt32>(m_entityGPUList.size()),
		});

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = setCount,
		.poolSizeCount = static_cast<UInt32>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(m_logicDevice, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return;

	// Create a single shared VulkanRasterizationMaterial for the override material
	m_sharedRasterMaterial = std::make_shared<Rasterization::VulkanRasterizationMaterial>(overrideMaterial, m_logicDevice);
	if (m_sharedRasterMaterial->Initialize(m_descriptorPool) == false)
		return;

	m_sharedRasterMaterial->WriteBuffer(HLSL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffer, m_transformStride);
	m_sharedRasterMaterial->WriteBuffer(HLSL_CAMERA_DATA_NAME, m_cameraBuffer, m_cameraStride);
	m_sharedRasterMaterial->WriteBuffer(HLSL_LIGHT_DATA_NAME, m_lightBuffer, m_lightStride);

	// Build a pipeline module per mesh-renderer entity using the shared material
	for (auto& entityGPU : m_entityGPUList) {
		auto mesh = ExtractMeshFromGameEntity(entityGPU.gameEntity);
		if (mesh == nullptr)
			continue;

		auto rasterizationModule = std::make_shared<Rasterization::VulkanRasterizationPipelineModule>(m_logicDevice);
		if (rasterizationModule->Initialize(mesh, m_sharedRasterMaterial, m_renderPass) == false)
			continue;

		rasterizationModule->SetDescriptorOffset(HLSL_OBJECT_TRANSFORM_DATA_NAME, entityGPU.index * m_transformStride);
		rasterizationModule->SetDescriptorOffset(HLSL_CAMERA_DATA_NAME, 0);
		rasterizationModule->SetDescriptorOffset(HLSL_LIGHT_DATA_NAME, 0);

		m_rasterizationModules.push_back(rasterizationModule);
	}
}

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::SyncEntities(const ref<Scene>& scene)
{
	const auto& sceneEntities = scene->entities;

	std::set<ref<GameEntity>> currentSet(sceneEntities.begin(), sceneEntities.end());
	std::set<ref<GameEntity>> existingSet;
	for (auto& gpuData : m_entityGPUList)
		existingSet.insert(gpuData.gameEntity);

	std::vector<ref<GameEntity>> removed;
	for (auto& e : existingSet)
		if (currentSet.find(e) == currentSet.end())
			removed.push_back(e);

	std::vector<ref<GameEntity>> added;
	for (auto& e : currentSet)
		if (existingSet.find(e) == existingSet.end())
			added.push_back(e);

	// Remove stale entries
	for (auto& entity : removed) {
		m_entityGPUList.erase(
			std::remove_if(m_entityGPUList.begin(), m_entityGPUList.end(),
				[&entity](const VKEntityGPUData& d) { return d.gameEntity == entity; }),
			m_entityGPUList.end());
	}

	// Upload and append new entities
	if (!added.empty()) {
		UploadMeshesToGPU(added);
		UInt32 nextIndex = static_cast<UInt32>(m_entityGPUList.size());
		for (auto& entity : added) {
			m_entityGPUList.push_back({ entity, nextIndex });
			nextIndex++;
		}
	}

	// Re-allocate transform buffer to fit the updated list size
	if (m_transformBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(m_logicDevice, m_transformBuffer, nullptr);
		vkFreeMemory(m_logicDevice, m_transformBufferMemory, nullptr);
	}

	m_bufferFactory->CreateBuffer(
		sizeof(TransformGPU),
		m_entityGPUList.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&m_transformBuffer, &m_transformBufferMemory, &m_transformStride);

	UploadMeshesToGPU(sceneEntities);
	InitializePipelines(m_overrideMaterial);
}

ref<LuxonEngine::Mesh> LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::ExtractMeshFromGameEntity(const ref<GameEntity>& entity)
{
	auto renderer = entity->GetRenderer();

	if (renderer != nullptr) {
		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(renderer);
		if (meshRenderer != nullptr)
			return meshRenderer->GetMesh();

		auto gbufferRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(renderer);
		if (gbufferRenderer != nullptr)
			return gbufferRenderer->GetMesh();
	}

	auto rtComponent = entity->GetRayTracingComponent();

	if (rtComponent != nullptr)
		return rtComponent->GetMesh();

	return nullptr;
}

std::vector<ref<LuxonEngine::Rendering::Vulkan::MeshShading::VulkanMeshPipelineModule>> LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::CreateSpikeMeshPipelines()
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

void LuxonEngine::Rendering::Vulkan::VulkanEditorGraphicContext::DestroyMeshStorageBuffers()
{
	// the buffers only share the memory of the mesh buffers, the memory is released with the mesh
	for (auto& storageBuffer : m_meshStorageBuffers)
		vkDestroyBuffer(m_logicDevice, storageBuffer, nullptr);

	m_meshStorageBuffers.clear();
}
