#include "EngineAPI.h"
#include <VulkanAPI.h>
#include <DX12API.h>

Render::GPUDeviceManager* CreateGPUApplication(Graphic_API graphicApi)
{
	switch (graphicApi) {
	case DIRECTX_12:
		return CreateDX12Application();
	case VULKAN:
		return CreateVulkanApplication();
	default:
		return nullptr;
	}
}
