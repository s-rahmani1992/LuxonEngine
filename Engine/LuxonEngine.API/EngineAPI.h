#pragma once
#include <Rendering/GPUDeviceManager.h>
#include <Rendering/GraphicContext.h>
#include <Rendering/ShaderRegistery.h>
#include <Rendering/MaterialFactory.h>
#include <Rendering/MeshRenderer.h>
#include <Core/Logger.h>
#include <BasicTypes.h>
#include <Platform/GraphicWindow.h>
#include <Core/Transform.h>
#include <Core/Camera/PerspectiveCamera.h>
#include <Core/Light/Lights.h>
#include <Core/Scene.h>
#include <Core/ShapeBuilder.h>
#include <Core/Mesh.h>

#define LUXON_ENGINE_API extern "C" __declspec(dllexport)

namespace Render = LuxonEngine::Rendering;

enum Graphic_API {
	DIRECTX_12,
	VULKAN,
};

LUXON_ENGINE_API Render::GPUDeviceManager* CreateGPUApplication(Graphic_API graphicApi);