#include "VulkanAPI.h"
#include "Core/VulkanDeviceManager.h"

Render::GPUDeviceManager* CreateVulkanApplication()
{
	auto vkApp = new Render::Vulkan::VulkanDeviceManager();
	vkApp->Initialize();
	return vkApp;
}
