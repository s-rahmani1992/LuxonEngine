#include "DemoAPI.h"
#include <Platform/Application.h>
#include <Platform/GraphicWindow.h>
#include <DX12GPUDeviceManager.h>
#include <Core/VulkanDeviceManager.h>
#include <Rendering/GraphicContext.h>
#include "Rendering/GPUAssetManager.h"
#include "SceneBuilder.h"
#include <Core/Scene.h>

namespace OS = QuantumEngine::Platform; 
namespace DX12 = QuantumEngine::Rendering::DX12;
namespace VK = QuantumEngine::Rendering::Vulkan;

bool Run_Simple_Scene(HWND parentWindow, Graphics_API graphicApi, RenderMode renderMode)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));

	ref<Render::GPUDeviceManager> gpuDevice;
	std::wstring graphicAPIStr;

	switch (graphicApi) {
	case DIRECTX_12:
		gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
		graphicAPIStr = L"DirectX 12";
		break;
	case VULKAN:
		gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
		graphicAPIStr = L"Vulkan";
		break;
	default:
		return false;
	}

	std::wstring renderModeString = renderMode == HYBRID ? L"Rasterization" : L"Ray Tracing";
	
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Simple Scene ---- " + renderModeString + L" ---- " + graphicAPIStr, .parentWinHandle = parentWindow });
	ref<Render::GraphicContext> gpuContext;
	switch (renderMode) {
	case HYBRID:
		gpuContext = gpuDevice->CreateHybridContextForWindows(win);		
		break;
	case VULKAN:
		gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
		break;
	default:
		return false;
	}

	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildSimpleLightScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	assetManager->UnloadAssets();
	DestroyWindow(win->GetHandle());
	Platform::Application::Release();


	return true;
}

bool Run_Reflection_Scene(HWND parentWindow, Graphics_API graphicApi, RenderMode renderMode)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));

	ref<Render::GPUDeviceManager> gpuDevice;
	std::wstring graphicAPIStr;

	switch (graphicApi) {
	case DIRECTX_12:
		gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
		graphicAPIStr = L"DirectX 12";
		break;
	case VULKAN:
		gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
		graphicAPIStr = L"Vulkan";
		break;
	default:
		return false;
	}

	std::wstring renderModeString = renderMode == HYBRID ? L"Hybrid" : L"Ray Tracing";

	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Reflection Scene ---- " + renderModeString + L" ---- " + graphicAPIStr, .parentWinHandle = parentWindow });
	ref<Render::GraphicContext> gpuContext;
	switch (renderMode) {
	case HYBRID:
		gpuContext = gpuDevice->CreateHybridContextForWindows(win);
		break;
	case VULKAN:
		gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
		break;
	default:
		return false;
	}

	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildReflectionScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	assetManager->UnloadAssets();
	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Shadow_Scene(HWND parentWindow, Graphics_API graphicApi)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));

	ref<Render::GPUDeviceManager> gpuDevice;
	std::wstring graphicAPIStr;

	switch (graphicApi) {
	case DIRECTX_12:
		gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
		graphicAPIStr = L"DirectX 12";
		break;
	case VULKAN:
		gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
		graphicAPIStr = L"Vulkan";
		break;
	default:
		return false;
	}

	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Shadow Scene ---- Ray Tracing ---- " + graphicAPIStr, .parentWinHandle = parentWindow });
	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);

	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildShadowScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	assetManager->UnloadAssets();
	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Refraction_Scene(HWND parentWindow, Graphics_API graphicApi)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));

	ref<Render::GPUDeviceManager> gpuDevice;
	std::wstring graphicAPIStr;

	switch (graphicApi) {
	case DIRECTX_12:
		gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
		graphicAPIStr = L"DirectX 12";
		break;
	case VULKAN:
		gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
		graphicAPIStr = L"Vulkan";
		break;
	default:
		return false;
	}

	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Refraction Scene ---- Ray Tracing ---- " + graphicAPIStr, .parentWinHandle = parentWindow });
	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);

	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildRefractionScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	assetManager->UnloadAssets();
	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Complete_Scene(HWND parentWindow, Graphics_API graphicApi)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));

	ref<Render::GPUDeviceManager> gpuDevice;
	std::wstring graphicAPIStr;

	switch (graphicApi) {
	case DIRECTX_12:
		gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
		graphicAPIStr = L"DirectX 12";
		break;
	case VULKAN:
		gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
		graphicAPIStr = L"Vulkan";
		break;
	default:
		return false;
	}

	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Complete Scene ---- Ray Tracing ---- " + graphicAPIStr, .parentWinHandle = parentWindow });
	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);

	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildComplexScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	assetManager->UnloadAssets();
	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}
