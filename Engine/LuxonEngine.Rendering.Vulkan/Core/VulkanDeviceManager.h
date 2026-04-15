#pragma once

#include "vulkan-pch.h"
#include "Rendering/GPUDeviceManager.h"

namespace QuantumEngine {
	namespace Platform {
		class GraphicWindow;
	}

	namespace Rendering {
		class GraphicContext;
	}
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanBufferFactory;

	class VulkanDeviceManager : public GPUDeviceManager
	{
	public:
		virtual bool Initialize() override;
		virtual ref<GraphicContext> CreateHybridContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window) override;
		virtual ref<GraphicContext> CreateRayTracingContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window) override;
		virtual ref<GPUAssetManager> CreateAssetManager() override;
		virtual ref<ShaderRegistery> CreateShaderRegistery() override;
		virtual ref<MaterialFactory> CreateMaterialFactory() override;
		~VulkanDeviceManager();
		ref<VulkanBufferFactory> GetBufferFactory() const { return m_bufferFactory; }
		VkInstance GetVKInstance() const { return m_instance; }
		VkDevice GetGraphicDevice() const { return m_graphicDevice; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
		VkQueue GetGraphicsQueue() const;
		UInt32 GetGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
		VkPhysicalDeviceAccelerationStructurePropertiesKHR* GetAccelerationStructureProperties() { return &m_accelProps; }
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR* GetRayTracingPipelineProperties() { return &m_rtPipelineProps; }
		static VulkanDeviceManager* Instance() { return s_instance; }
	private:

#if defined(_DEBUG)
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

		static VulkanDeviceManager* s_instance;

		bool CheckQueueSupport(VkPhysicalDevice device);
		Int32 FindQueueFamilies(VkPhysicalDevice device, UInt32 flag);
		Int32 FindPresentFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions);
		bool CheckDeviceSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_graphicDevice;
		UInt32 m_graphicsQueueFamilyIndex;
		UInt32 m_surfaceQueueFamilyIndex;
		VkPhysicalDeviceAccelerationStructurePropertiesKHR m_accelProps;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtPipelineProps;
		ref<VulkanBufferFactory> m_bufferFactory;
	};
}