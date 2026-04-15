#include "vulkan-pch.h"
#include "VulkanGraphicContext.h"
#include "Platform/GraphicWindow.h"
#include "Core/Scene.h"
#include "VulkanBufferFactory.h"
#include "Core/Transform.h"
#include "VulkanUtilities.h"
#include "VulkanAssetManager.h"
#include "VulkanShaderRegistery.h"
#include "Core/VulkanDeviceManager.h"

QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::VulkanGraphicContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window)
	:m_instance(vkInstance), 
	m_logicDevice(VulkanDeviceManager::Instance()->GetGraphicDevice()),
	m_physicalDevice(VulkanDeviceManager::Instance()->GetPhysicalDevice()),
	m_graphicsQueueFamilyIndex(VulkanDeviceManager::Instance()->GetGraphicsQueueFamilyIndex()), m_window(window)
	, m_bufferFactory(VulkanDeviceManager::Instance()->GetBufferFactory())
{
	vkGetDeviceQueue(m_logicDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicDevice, surfaceQueueFamilyIndex, 0, &m_presentQueue);
}

QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::~VulkanGraphicContext()
{
	vkDestroyBuffer(m_logicDevice, m_lightBuffer, nullptr);
	vkFreeMemory(m_logicDevice, m_lightBufferMemory, nullptr);
	vkDestroyBuffer(m_logicDevice, m_cameraBuffer, nullptr);
	vkFreeMemory(m_logicDevice, m_cameraBufferMemory, nullptr);
	
	vkDestroySemaphore(m_logicDevice, m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_logicDevice, m_renderFinishedSemaphore, nullptr);
	vkDestroyFence(m_logicDevice, m_fence, nullptr);
	vkDestroyCommandPool(m_logicDevice, m_commandPool, nullptr);

	for(auto& imageView : m_swapChainImageViews) {
		vkDestroyImageView(m_logicDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_logicDevice, m_swapChain, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

void QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::RegisterAssetManager(const ref<GPUAssetManager>& assetManager)
{
	m_assetManager = std::dynamic_pointer_cast<VulkanAssetManager>(assetManager);
}

void QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery)
{
	m_shaderRegistery = std::dynamic_pointer_cast<VulkanShaderRegistery>(shaderRegistery);
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::InitializeLightBuffer(const SceneLightData& lightData)
{
	UInt32 lightSize = 10 * (sizeof(DirectionalLight) + sizeof(PointLight)) + 2 * sizeof(UInt32);
	bool result =  m_bufferFactory->CreateBuffer(lightSize, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&m_lightBuffer, &m_lightBufferMemory, &m_lightStride);

	if(result == false)
		return false;

	void* data;
	vkMapMemory(m_logicDevice, m_lightBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
	Byte* pData = (Byte*)data;
	std::memcpy(pData, (void*)lightData.directionalLights.data(), lightData.directionalLights.size() * sizeof(DirectionalLight));
	pData += 10 * (sizeof(DirectionalLight));
	std::memcpy(pData, (void*)lightData.pointLights.data(), lightData.pointLights.size() * sizeof(PointLight));
	pData += 10 * (sizeof(PointLight));
	UInt32* sizeData = (UInt32*)(pData);
	*sizeData = lightData.directionalLights.size();
	sizeData++;
	*sizeData = lightData.pointLights.size();
	vkUnmapMemory(m_logicDevice, m_lightBufferMemory);
	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::UpdateCameraBuffer()
{
	void* data;
	m_cameraGPU.inverseProjectionMatrix = m_camera->GetTransform()->Matrix() * m_camera->InverseProjectionMatrix();
	m_cameraGPU.viewMatrix = m_camera->ViewMatrix();
	m_cameraGPU.position = m_camera->GetTransform()->Position();

	vkMapMemory(m_logicDevice, m_cameraBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
	std::memcpy(data, &m_cameraGPU, sizeof(CameraGPU));
	vkUnmapMemory(m_logicDevice, m_cameraBufferMemory);
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::InitializeSwapChain(VkImageUsageFlags useFlag)
{
	// Create Surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = m_window->GetHandle(),
	};

	VkResult result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface);

	if (result != VK_SUCCESS)
		return false;

	// Create Swap Chain
	uint32_t formatCount;
	std::vector<VkSurfaceFormatKHR> formats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
	formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

	for (const auto& availableFormat : formats) {
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			m_swapChainFormat = availableFormat;
			break;
		}
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &m_swapChainCapability);

	VkSwapchainCreateInfoKHR swapChainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = m_surface,
		.minImageCount = m_swapChainCapability.minImageCount,
		.imageFormat = m_swapChainFormat.format,
		.imageColorSpace = m_swapChainFormat.colorSpace,
		.imageExtent = m_swapChainCapability.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = useFlag,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = m_swapChainCapability.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	result = vkCreateSwapchainKHR(m_logicDevice, &swapChainCreateInfo, nullptr, &m_swapChain);

	if (result != VK_SUCCESS)
		return false;

	// Create Image Views for Swap chain images
	UInt32 imageCount;
	vkGetSwapchainImagesKHR(m_logicDevice, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_logicDevice, m_swapChain, &imageCount, m_swapChainImages.data());
	m_swapChainImageViews.resize(imageCount);

	for (UInt32 i = 0; i < m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo imageViewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0, // TODO Check if any flags are needed
			.image = m_swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_swapChainFormat.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		vkCreateImageView(m_logicDevice, &imageViewCreateInfo, nullptr, &m_swapChainImageViews[i]);
	}
	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::InitializeCommandObjects()
{
	// Create Command Pool and Command Buffer
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_graphicsQueueFamilyIndex,
	};

	auto result = vkCreateCommandPool(m_logicDevice, &poolInfo, nullptr, &m_commandPool);
	if (result != VK_SUCCESS)
		return false;

	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	result = vkAllocateCommandBuffers(m_logicDevice, &allocInfo, &m_commandBuffer);
	if (result != VK_SUCCESS)
		return false;

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::InitializeFencesAndSemaphores()
{
	// Create Fences and Semaphores
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0, // TODO Check if any flags are needed
	};

	auto result = vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
	if (result != VK_SUCCESS)
		return false;

	result = vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
	if (result != VK_SUCCESS)
		return false;

	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};

	result = vkCreateFence(m_logicDevice, &fenceInfo, nullptr, &m_fence);
	if (result != VK_SUCCESS)
		return false;

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::InitializeCameraBuffer(const ref<Camera>& camera)
{
	// Create Uniform Buffer for Camera
	m_bufferFactory->CreateBuffer(sizeof(CameraGPU), 1
		, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_cameraBuffer, &m_cameraBufferMemory, &m_cameraStride);

	m_camera = camera;
	m_cameraGPU.viewMatrix = m_camera->ViewMatrix();
	m_cameraGPU.projectionMatrix = m_camera->ProjectionMatrix();
	m_cameraGPU.projectionMatrix.SetValue(1, 1, -m_cameraGPU.projectionMatrix(1, 1));
	m_cameraGPU.inverseProjectionMatrix = m_camera->InverseProjectionMatrix();

	return true;
}
