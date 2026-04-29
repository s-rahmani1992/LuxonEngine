#pragma once
#include <Rendering/GPUDeviceManager.h>
#include <string>

namespace Render = LuxonEngine::Rendering;

extern "C" __declspec(dllexport) Render::GPUDeviceManager* CreateVulkanApplication();