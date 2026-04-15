// vulkan-pch.h: This is a precompiled header file.

#ifndef VULKAN_PCH_H
#define VULKAN_PCH_H

#define VK_USE_PLATFORM_WIN32_KHR
#include <BasicTypes.h>
#include <vulkan/vulkan.h>
#include <dxcapi.h>
#include "Core/spirv_reflect.h"
#include <wrl/client.h>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "dxcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "libboost_json-vc143-mt-gd-x64-1_90.lib")
#pragma comment(lib, "libboost_container-vc143-mt-gd-x64-1_90.lib")
#else
#pragma comment(lib, "libboost_json-vc143-mt-x64-1_90.lib")
#pragma comment(lib, "libboost_container-vc143-mt-x64-1_90.lib")
#endif


#endif //VULKAN_PCH_H
