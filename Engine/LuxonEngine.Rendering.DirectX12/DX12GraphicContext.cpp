#include "pch.h"
#include "DX12GraphicContext.h"
#include "DX12CommandExecuter.h"
#include "Platform/GraphicWindow.h"
#include "DX12MeshController.h"
#include "Core/GameEntity.h"
#include "Core/Transform.h"
#include "Core/Camera/Camera.h"
#include "DX12Utilities.h"
#include "DX12ShaderRegistery.h"
#include "DX12AssetManager.h"
#include "Rendering/Renderer.h"
#include "Rendering/RayTracingComponent.h"
#include "Core/Scene.h"

QuantumEngine::Rendering::DX12::DX12GraphicContext::DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window)
	:m_bufferCount(bufferCount), m_commandExecuter(commandExecuter), m_window(window),
	m_renderBuffers(std::vector<ComPtr<ID3D12Resource2>>(bufferCount)),
	m_rtvHandles(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(bufferCount))
{
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RegisterAssetManager(const ref<GPUAssetManager>& assetManager)
{
	m_assetManager = std::dynamic_pointer_cast<DX12AssetManager>(assetManager);
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery)
{
	m_shaderRegistery = std::dynamic_pointer_cast<DX12ShaderRegistery>(shaderRegistery);
}

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::InitializeCommandObjects(const ComPtr<ID3D12Device10>& device)
{
	m_device = device;

	//command allocator
	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator))))
		return false;

	//Command List
	if (FAILED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_commandList))))
		return false;

	return true;
}

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::InitializeSwapChain(const ComPtr<IDXGIFactory7>& factory)
{
	//Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc1
	{
	.Width = m_window->GetWidth(),
	.Height = m_window->GetHeight(),
	.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	.Stereo = false,
	.SampleDesc = DXGI_SAMPLE_DESC
		{
		.Count = 1,
		.Quality = 0,
		},
	.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
	.BufferCount = m_bufferCount,
	.Scaling = DXGI_SCALING_STRETCH,
	.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
	.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc{ 0 };
	swapChainFullScreenDesc.Windowed = true;

	ComPtr<IDXGISwapChain1> swapChain1;

	if (FAILED(factory->CreateSwapChainForHwnd(m_commandExecuter->GetQueue().Get(), m_window->GetHandle(), &swapChainDesc1, &swapChainFullScreenDesc, nullptr, &swapChain1))) {
		return false;
	}

	if (FAILED(swapChain1.As(&m_swapChain)))
		return false;

	// Create Heap for Render Target view
	D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc{
	.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	.NumDescriptors = m_bufferCount,
	.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	.NodeMask = 0,
	};
	if (FAILED(m_device->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&m_rtvDescriptorHeap))))
		return false;

	auto firstHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto handleIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (size_t i = 0; i < m_bufferCount; i++) {
		m_rtvHandles[i] = firstHandle;
		m_rtvHandles[i].ptr += i * handleIncrementSize;
	}

	//create render buffers and render target views
	for (size_t i = 0; i < m_bufferCount; i++) {
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderBuffers[i]));
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = D3D12_TEX2D_RTV
			{
				.MipSlice = 0,
				.PlaneSlice = 0
			},
		};
		m_device->CreateRenderTargetView(m_renderBuffers[i].Get(), &rtv_desc, m_rtvHandles[i]);
	}

	return true;
}

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::InitializeCamera(const ref<Camera>& camera)
{
	m_camera = camera;
	m_camData.projectionMatrix = camera->ProjectionMatrix();
	m_camData.inverseProjectionMatrix = camera->InverseProjectionMatrix();

	auto cameraResourceDesc = ResourceUtilities::GetCommonBufferResourceDesc(CONSTANT_BUFFER_ALIGHT(sizeof(CameraGPU)), D3D12_RESOURCE_FLAG_NONE);

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
		&cameraResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cameraBuffer))))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cameraHeap)))) {
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC transformViewDesc;
	transformViewDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	transformViewDesc.SizeInBytes = cameraResourceDesc.Width;

	m_device->CreateConstantBufferView(&transformViewDesc, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::InitializeLight(const SceneLightData& lights)
{
	return m_lightManager.Initialize(lights, m_device);
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::InitializeEntityGPUData(const std::vector<ref<GameEntity>>& gameEntities)
{
	auto transformResourceDesc = ResourceUtilities::GetCommonBufferResourceDesc(CONSTANT_BUFFER_ALIGHT(sizeof(TransformGPU)), D3D12_RESOURCE_FLAG_NONE);

	// Create Transform Resources and Rasterization Heap 
	for (auto& gameEntity : gameEntities) {
		ComPtr<ID3D12Resource2> transformResource;

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
			&transformResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformResource))))
		{
			continue;
		}

		m_entityGPUData.push_back(DX12EntityGPUData{
			.gameEntity = gameEntity,
			.transformResource = transformResource,
			});
	}
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::UploadTexturesAndMeshes(const ref<Scene>& scene)
{
	std::set<ref<Mesh>> uniqueMeshes;

	for (auto& entity : scene->entities) {
		auto mesh = entity->GetRenderer()->GetMesh();
		
		if(mesh == nullptr)
			continue;

		uniqueMeshes.insert(mesh);

		auto rtcomponent = entity->GetRayTracingComponent();

		if(rtcomponent != nullptr)
			uniqueMeshes.insert(rtcomponent->GetMesh());
	}

	m_assetManager->UploadMeshesToGPU(std::vector<ref<Mesh>>(uniqueMeshes.begin(), uniqueMeshes.end()));
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::UpdateDataHeaps()
{
	m_camData.inverseProjectionMatrix = m_camera->GetTransform()->Matrix() * m_camera->InverseProjectionMatrix();
	m_camData.viewMatrix = m_camera->ViewMatrix();
	m_camData.position = m_camera->GetTransform()->Position();

	void* camData;
	m_cameraBuffer->Map(0, nullptr, &camData);
	std::memcpy(camData, &m_camData, sizeof(CameraGPU));
	m_cameraBuffer->Unmap(0, nullptr);

	for (auto& entity : m_entityGPUData) {
		m_transformData.modelMatrix = entity.gameEntity->GetTransform()->Matrix();
		m_transformData.modelViewMatrix = m_camData.viewMatrix * m_transformData.modelMatrix;
		m_transformData.rotationMatrix = entity.gameEntity->GetTransform()->RotateMatrix();
		void* data;
		entity.transformResource->Map(0, nullptr, &data);
		std::memcpy(data, &m_transformData, sizeof(TransformGPU));
		entity.transformResource->Unmap(0, nullptr);
	}
}
