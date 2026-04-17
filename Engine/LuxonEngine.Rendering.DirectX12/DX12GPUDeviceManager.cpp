#include "pch.h"
#include "Platform/GraphicWindow.h"
#include "DX12GPUDeviceManager.h"
#include "DX12CommandExecuter.h"
#include "DX12RayTracingContext.h"
#include "DX12HybridContext.h"
#include "DX12AssetManager.h"
#include "DX12ShaderRegistery.h"
#include "DX12MaterialFactory.h"

bool LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::Initialize()
{
	// Create Debug layer in Debug build
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_d3d12_dubug)))) {
		m_d3d12_dubug->EnableDebugLayer();

		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug)))) {
			m_dxgi_debug->EnableLeakTrackingForThread();
		}
	}
#endif

	//Find High Performance, DirectX 12 compatible GPU
	if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)))) {
		return false;
	}

	bool adapterFound = false;

	//Loop through available adapters
	for (int adapterId = 0; m_factory->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)) != DXGI_ERROR_NOT_FOUND; adapterId++) {
		DXGI_ADAPTER_DESC3 desc;
		m_adapter->GetDesc3(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // Don't select the Basic Render Driver adapter.
			continue;

		// Checking for Directx 12 capability and creating device at the same time.
		if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device)))) {
			adapterFound = true;
			break;
		}
	}

	if (!adapterFound)
		return false;

	m_shaderRegistry = std::make_shared<DX12ShaderRegistery>();
	m_shaderRegistry->Initialize(m_device);
}

ref<LuxonEngine::Rendering::GraphicContext> LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::CreateHybridContextForWindows(ref<LuxonEngine::Platform::GraphicWindow>& window)
{
	ref<DX12CommandExecuter> cmdExecuter = CreateCommandExecuter();
	ref<DX12GraphicContext> context = std::make_shared< DX12HybridContext>(2, cmdExecuter, window);
	
	if (context->Initialize(m_device.Get(), m_factory))
		return context;

	return nullptr;
}

ref<LuxonEngine::Rendering::GraphicContext> LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::CreateRayTracingContextForWindows(ref<LuxonEngine::Platform::GraphicWindow>& window)
{
	ref<DX12CommandExecuter> cmdExecuter = CreateCommandExecuter();
	ref<DX12GraphicContext> context = std::make_shared<DX12RayTracingContext>(2, cmdExecuter, window);

	if (context->Initialize(m_device.Get(), m_factory))
		return context;

	return nullptr;
}

ref<LuxonEngine::Rendering::GPUAssetManager> LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::CreateAssetManager()
{
	auto assetManager = std::make_shared<DX12AssetManager>();

	if(assetManager->Initialize(m_device) == false)
		return nullptr;

	return assetManager;
}

ref<LuxonEngine::Rendering::ShaderRegistery> LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::CreateShaderRegistery()
{
	return m_shaderRegistry;
}

ref<LuxonEngine::Rendering::MaterialFactory> LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::CreateMaterialFactory()
{
	return std::make_shared<DX12MaterialFactory>();
}

LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::~DX12GPUDeviceManager()
{
#ifdef _DEBUG
	if (m_dxgi_debug) {
		OutputDebugStringW(L"Live Object Reports:\n");
		m_dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}
#endif
}

ref<LuxonEngine::Rendering::DX12::DX12CommandExecuter> LuxonEngine::Rendering::DX12::DX12GPUDeviceManager::CreateCommandExecuter()
{
	//Command Queue
	ComPtr<ID3D12CommandQueue> cmdqueue;
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{
	.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
	.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	.NodeMask = 0,
	};

	if (FAILED(m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdqueue))))
		return nullptr;

	//fence
	ComPtr<ID3D12Fence1> fence;
	if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		return nullptr;

	return std::make_shared<DX12CommandExecuter>(cmdqueue, fence);
}
