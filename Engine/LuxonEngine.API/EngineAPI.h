#pragma once
#include <Rendering/GPUDeviceManager.h>

#define LUXON_ENGINE_API extern "C" __declspec(dllexport)

namespace Render = LuxonEngine::Rendering;

enum Graphic_API {
	DIRECTX_12,
	VULKAN,
};

LUXON_ENGINE_API Render::GPUDeviceManager* CreateGPUApplication(Graphic_API graphicApi);