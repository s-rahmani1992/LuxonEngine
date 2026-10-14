#include "vulkan-pch.h"
#include "VulkanHybridContext.h"
#include "Core/Scene.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "Core/Transform.h"
#include "Rendering/Renderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/SplineRenderer.h"
#include "Rendering/SpikeMeshRenderer.h"
#include "Rendering/SurfaceInstanceRenderer.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include "Rendering/RayTracingComponent.h"
#include <Rendering/ShaderRegistery.h>
#include "VulkanAssetManager.h"
#include "VulkanBufferFactory.h"
#include "VulkanPipelineFactory.h"
#include "VulkanRenderPass.h"
#include "VulkanComputePass.h"
#include "VulkanDepthTexture.h"
#include "RendererSetup/VulkanGBufferRendererSetup.h"
#include "RendererSetup/VulkanMeshRendererSetup.h"
#include "RendererSetup/VulkanSplineRendererSetup.h"
#include "RendererSetup/VulkanSpikeMeshRendererSetup.h"
#include "RendererSetup/VulkanSurfaceInstanceRendererSetup.h"
#include "Core/Logger.h"
#include <set>

LuxonEngine::Rendering::Vulkan::VulkanHybridContext::VulkanHybridContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window,
	const ref<VulkanPipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery)
	:VulkanGraphicContext(vkInstance, surfaceQueueFamilyIndex, window), m_shaderProgramRegistery(shaderRegistery), m_pipelineFactory(pipelineFactory)
{
	m_swapChainUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

LuxonEngine::Rendering::Vulkan::VulkanHybridContext::~VulkanHybridContext()
{
	m_passSequence.Clear();
	m_computePass.reset();
	m_mainPass.reset();

	m_setupRegistry.Teardown();
	DestroyMeshStorageBuffers();

	DestroyEntityTransformBuffers(m_transformBuffers);
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::Initialize()
{
	m_setupRegistry.Register<GBufferRTReflectionRenderer, VulkanGBufferRendererSetup>();
	m_setupRegistry.Register<MeshRenderer, VulkanMeshRendererSetup>();
	m_setupRegistry.Register<SplineRenderer, VulkanSplineRendererSetup>();
	m_setupRegistry.Register<SpikeMeshRenderer, VulkanSpikeMeshRendererSetup>();
	m_setupRegistry.Register<SurfaceInstanceRenderer, VulkanSurfaceInstanceRendererSetup>();

	if (InitializeSurface() == false)
		return false;

	if (InitializeSwapChain(m_swapChainUsageFlags) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	if(InitializeCommandObjects() == false)
		return false;

	if(InitializeFencesAndSemaphores() == false)
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

	if (CreateEntityTransformBuffers((UInt32)scene->entities.size(), m_transformBuffers) == false)
		return false;

	m_sceneEntities = scene->entities;
	m_entityGPUList.clear();
	m_entityGPUList.reserve(scene->entities.size());

	UInt32 index = 0;
	for (auto& entity : scene->entities) {
		m_entityGPUList.push_back({ entity, index });
		index++;
	}

	return InitializePasses();
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::Render()
{
	if (m_mainPass == nullptr)
		return;

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

	m_setupRegistry.Update();

	m_mainPass->SetImageIndex(imageIndex);
	m_passSequence.Execute(m_commandBuffer, imageIndex);
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
	m_passSequence.Clear();
	m_computePass.reset();
	m_mainPass.reset();
	m_setupRegistry.Teardown();
	DestroyMeshStorageBuffers();

	m_depthTexture.reset();

	RecreateSwapChain(m_swapChainUsageFlags);

	InitializeDepthBuffer();
	InitializePasses();
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::InitializePasses()
{
	m_passSequence.Clear();
	m_computePass.reset();
	m_mainPass.reset();
	m_setupRegistry.Teardown();
	DestroyMeshStorageBuffers();

	if (InitializeRenderPass() == false)
		return false;

	m_computePass = std::make_unique<VulkanComputePass>();

	RunRendererSetups();

	return BuildPassSequence();
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::RunRendererSetups()
{
	m_rendererEntities.clear();

	for (auto& entityGPU : m_entityGPUList) {
		m_rendererEntities.push_back(VulkanRendererEntity{
			.entity = entityGPU.gameEntity.get(),
			.renderer = entityGPU.gameEntity->GetRenderer().get(),
			.transformBuffer = m_transformBuffers[entityGPU.index].buffer,
		});
	}

	m_setupContext = VulkanPassSetupContext();
	m_setupContext.device = m_logicDevice;
	m_setupContext.pipelineFactory = m_pipelineFactory.get();
	m_setupContext.bufferFactory = m_bufferFactory.get();
	m_setupContext.assetManager = m_assetManager.get();
	m_setupContext.getInternalProgram = [this](const std::string& identifier) { return GetInternalProgram(identifier); };
	m_setupContext.renderExtent = m_swapChainCapability.currentExtent;
	m_setupContext.depthFormat = m_depthFormat;
	m_setupContext.backgroundColor = { m_clearColor.color.float32[0], m_clearColor.color.float32[1], m_clearColor.color.float32[2], m_clearColor.color.float32[3] };
	m_setupContext.sceneEntities = &m_sceneEntities;
	m_setupContext.allEntities = std::span<const VulkanRendererEntity>(m_rendererEntities);
	m_setupContext.cameraBuffer = m_cameraBuffer;
	m_setupContext.cameraStride = m_cameraStride;
	m_setupContext.lightBuffer = m_lightBuffer;
	m_setupContext.lightStride = m_lightStride;
	m_setupContext.meshStorageBuffers = &m_meshStorageBuffers;
	m_setupContext.computePass = m_computePass.get();
	m_setupContext.mainRenderPass = m_mainPass.get();

	m_setupRegistry.Setup(m_rendererEntities, m_setupContext);
}

bool LuxonEngine::Rendering::Vulkan::VulkanHybridContext::BuildPassSequence()
{
	m_passSequence.Clear();
	m_passSequence.SetBackBuffers(m_swapChainImages);

	if (m_setupContext.gBufferRenderPass != nullptr)
		m_passSequence.AddPass(m_setupContext.gBufferRenderPass);

	m_passSequence.AddPass(m_computePass.get());
	m_passSequence.AddPass(m_mainPass.get());

	if (m_passSequence.Compile() == false) {
		Logger::LogError("Failed to create the resource barriers of the passes");
		return false;
	}

	return true;
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
	m_mainPass = std::make_unique<VulkanRenderPass>();

	VulkanRenderPassParams params;
	params.clearColors = { { m_clearColor.color.float32[0], m_clearColor.color.float32[1], m_clearColor.color.float32[2], m_clearColor.color.float32[3] } };
	params.depthTarget = m_depthTexture.get();
	params.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	if (m_mainPass->Initialize(m_logicDevice, m_swapChainFormat.format, m_swapChainCapability.currentExtent, m_swapChainImageViews, params) == false) {
		Logger::LogError("Failed to initialize the main render pass");
		m_mainPass.reset();
		return false;
	}

	return true;
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

LuxonEngine::Rendering::ShaderProgram* LuxonEngine::Rendering::Vulkan::VulkanHybridContext::GetInternalProgram(const std::string& identifier) const
{
	if (m_shaderProgramRegistery == nullptr)
		return nullptr;

	return m_shaderProgramRegistery->GetShaderProgram(identifier);
}

void LuxonEngine::Rendering::Vulkan::VulkanHybridContext::DestroyMeshStorageBuffers()
{
	for (auto& storageBuffer : m_meshStorageBuffers)
		vkDestroyBuffer(m_logicDevice, storageBuffer, nullptr);

	m_meshStorageBuffers.clear();
}
