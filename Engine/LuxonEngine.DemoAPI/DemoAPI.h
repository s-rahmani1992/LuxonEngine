#pragma once
#include <Windows.h>

#define DEMO_API extern "C" __declspec(dllexport)

enum Graphics_API {
	DIRECTX_12 = 0,
	VULKAN = 1,
};

enum RenderMode {
	HYBRID = 0,
	FULL_RAY_TRACING = 1,
};

DEMO_API bool Run_Simple_Scene(HWND parentWindow, Graphics_API graphicApi, RenderMode renderMode);

DEMO_API bool Run_Reflection_Scene(HWND parentWindow, Graphics_API graphicApi, RenderMode renderMode);

DEMO_API bool Run_Shadow_Scene(HWND parentWindow, Graphics_API graphicApi);

DEMO_API bool Run_Refraction_Scene(HWND parentWindow, Graphics_API graphicApi);

DEMO_API bool Run_Complete_Scene(HWND parentWindow, Graphics_API graphicApi);