#pragma once

#include "vulkan-pch.h"
#include "Rendering/GraphicContext.h"

namespace QuantumEngine {
	namespace Platform {
		class GraphicWindow;
	}
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanBufferFactory;
	class VulkanAssetManager;
	class VulkanShaderRegistery;

	struct TransformGPU {
	public:
		Matrix4 modelMatrix;
		Matrix4 rotationMatrix;
		Matrix4 modelViewMatrix;
	};

	class VulkanGraphicContext : public GraphicContext
	{
	public:
		VulkanGraphicContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		~VulkanGraphicContext();
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& assetManager) override;
		virtual void RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery) override;
		virtual bool PrepareScene(const ref<Scene>& scene) = 0;
		virtual void Render() = 0;

	private:

		ref<QuantumEngine::Platform::GraphicWindow> m_window;		
		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		
	protected:
		bool InitializeSwapChain(VkImageUsageFlags useFlag);
		bool InitializeCommandObjects();
		bool InitializeFencesAndSemaphores();
		bool InitializeCameraBuffer(const ref<Camera>& camera);
		bool InitializeLightBuffer(const SceneLightData& lightData);
		void UpdateCameraBuffer();

		VkDevice m_logicDevice;

		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue; 
		
		VkCommandPool m_commandPool;
		VkCommandBuffer m_commandBuffer;

		VkSemaphore m_imageAvailableSemaphore;
		VkSemaphore m_renderFinishedSemaphore;
		VkFence m_fence;

		VkSurfaceKHR m_surface;
		VkSurfaceFormatKHR m_swapChainFormat;
		VkSwapchainKHR m_swapChain;
		VkSurfaceCapabilitiesKHR m_swapChainCapability;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		
		UInt32 m_graphicsQueueFamilyIndex;

		ref<VulkanAssetManager> m_assetManager;
		ref<VulkanShaderRegistery> m_shaderRegistery;
		ref<VulkanBufferFactory> m_bufferFactory;

		ref<Camera> m_camera;
		VkBuffer m_cameraBuffer;
		VkDeviceMemory m_cameraBufferMemory;
		UInt32 m_cameraStride;
		CameraGPU m_cameraGPU;

		VkBuffer m_lightBuffer;
		VkDeviceMemory m_lightBufferMemory;
		UInt32 m_lightStride;
	};
}
