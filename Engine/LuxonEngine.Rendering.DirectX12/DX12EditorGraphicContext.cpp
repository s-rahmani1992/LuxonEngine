#include "pch.h"
#include "DX12EditorGraphicContext.h"
#include "Platform/GraphicWindow.h"
#include "DX12Utilities.h"
#include "DX12CommandExecuter.h"
#include "DX12GameEntityPipelineModule.h"
#include "Core/GameEntity.h"
#include "Rendering/MeshRenderer.h"
#include "DX12ShaderRegistery.h"
#include "Core/Scene.h"
#include "Rasterization/DX12RasterizationMaterial.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "DX12MaterialFactory.h"
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Rendering/RayTracingComponent.h>

bool LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	if (InitializeSwapChain(factory) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	return true;
}

bool LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::PrepareScene(const ref<Scene>& scene)
{
	m_scene = scene;

	if (InitializeCamera(scene->mainCamera) == false)
		return false;

	SceneLightData editorLightData;
	editorLightData.directionalLights.push_back(DirectionalLight{
		.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
		.direction = Vector3(2.0f, -6.0f, 2.0f),
		.intensity = 0.5f,
		});

	if (InitializeLight(editorLightData) == false)
		return false;

	// Build the shared override material from the solid color shader
	m_overrideMaterial = m_scene->rtGlobalMaterial;

	UploadTexturesAndMeshes(scene);
	InitializeEntityGPUData(scene->entities);
	InitializePipelines(m_overrideMaterial);

	return true;
}

void LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::Render()
{
	// Sync entity list before updating GPU data
	if (m_scene)
		SyncEntities();

	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	// Set Render Target
	auto currentBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER beginBarrier
	{
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
			.pResource = m_renderBuffers[currentBufferIndex].Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};
	m_commandList->ResourceBarrier(1, &beginBarrier);

	m_commandList->ClearDepthStencilView(m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	float clearColor[] = { 0.2f, 0.4f, 0.6f, 1.0f };
	auto dsvHandle = m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->ClearRenderTargetView(m_rtvHandles[currentBufferIndex], clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_rtvHandles[currentBufferIndex], false, &dsvHandle);

	// Viewport
	D3D12_VIEWPORT viewPort{};
	viewPort.Height = m_window->GetHeight();
	viewPort.Width = m_window->GetWidth();
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewPort);

	// Scissor Rect
	RECT scissorRect{};
	scissorRect.left = scissorRect.top = 0;
	scissorRect.right = m_window->GetWidth();
	scissorRect.bottom = m_window->GetHeight();
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Draw
	m_commandList->SetDescriptorHeaps(1, m_rasterHeap.GetAddressOf());
	for (auto& pipeline : m_rasterizationPipelines) {
		pipeline->Render(m_commandList, m_cameraHandle, m_lightHandle);
	}

	D3D12_RESOURCE_BARRIER endBarrier
	{
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
			.pResource = m_renderBuffers[currentBufferIndex].Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
			.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	m_commandList->ResourceBarrier(1, &endBarrier);

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

bool LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::InitializeDepthBuffer()
{
	D3D12_RESOURCE_DESC depthResourceDesc;
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = m_window->GetWidth();
	depthResourceDesc.Height = m_window->GetHeight();
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.MipLevels = 1;
	depthResourceDesc.Format = m_depthFormat;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.SampleDesc.Quality = 0;
	depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = m_depthFormat;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer))))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};

	if (FAILED(m_device->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&m_depthStencilvHeap))))
		return false;

	D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc{
		.Format = m_depthFormat,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D = D3D12_TEX2D_DSV{.MipSlice = 0},
	};

	m_device->CreateDepthStencilView(
		m_depthStencilBuffer.Get(),
		&depthViewDesc,
		m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::InitializePipelines(const ref<Material>& overrideMaterial)
{
	m_meshRendererData.clear();
	m_rasterizationPipelines.clear();

	// heap slots: camera + light + one transform per entity
	UInt32 rasterHeapSize = 2 + static_cast<UInt32>(m_entityGPUData.size());

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = rasterHeapSize,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};

	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rasterHeap));

	auto incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto cpuHandle = m_rasterHeap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandle = m_rasterHeap->GetGPUDescriptorHandleForHeapStart();

	// Camera CBV
	D3D12_CONSTANT_BUFFER_VIEW_DESC cameraViewDesc;
	cameraViewDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cameraViewDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(CameraGPU));
	m_device->CreateConstantBufferView(&cameraViewDesc, cpuHandle);
	m_cameraHandle = gpuHandle;

	cpuHandle.ptr += incrementSize;
	gpuHandle.ptr += incrementSize;

	// Light CBV
	D3D12_CONSTANT_BUFFER_VIEW_DESC lightViewDesc;
	lightViewDesc.BufferLocation = m_lightManager.GetResource()->GetGPUVirtualAddress();
	lightViewDesc.SizeInBytes = m_lightManager.GetResource()->GetDesc().Width;
	m_device->CreateConstantBufferView(&lightViewDesc, cpuHandle);
	m_lightHandle = gpuHandle;

	cpuHandle.ptr += incrementSize;
	gpuHandle.ptr += incrementSize;

	// Build a single shared DX12RasterizationMaterial wrapping the override material
	auto program = std::dynamic_pointer_cast<Rasterization::HLSLRasterizationProgram>(overrideMaterial->GetProgram());
	auto sharedRasterMaterial = std::make_shared<Rasterization::DX12RasterizationMaterial>(overrideMaterial, program);

	// Per-entity transform CBVs
	for (auto& entityGpu : m_entityGPUData) {
		auto mesh = ExtractMeshFromGameEntity(entityGpu.gameEntity);
		if(mesh == nullptr)
			continue;

		D3D12_CONSTANT_BUFFER_VIEW_DESC transformViewDesc;
		transformViewDesc.BufferLocation = entityGpu.transformResource->GetGPUVirtualAddress();
		transformViewDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(TransformGPU));
		m_device->CreateConstantBufferView(&transformViewDesc, cpuHandle);

		m_meshRendererData.push_back(DX12MeshRendererGPUData{
			.meshRenderer = std::make_shared<MeshRenderer>(mesh, nullptr),
			.material = sharedRasterMaterial,
			.transformResource = entityGpu.transformResource,
			.transformHandle = gpuHandle,
			});

		cpuHandle.ptr += incrementSize;
		gpuHandle.ptr += incrementSize;
	}

	for (auto& meshRenderData : m_meshRendererData) {
		if(meshRenderData.meshRenderer->GetMesh() == nullptr)
			continue;

		auto pipeline = std::make_shared<DX12GameEntityPipelineModule>();
		if (pipeline->Initialize(m_device, meshRenderData, m_depthFormat) == false)
			continue;

		m_rasterizationPipelines.push_back(pipeline);
	}
}

void LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::SyncEntities()
{
	const auto& sceneEntities = m_scene->entities;

	// Collect current entity set
	std::set<ref<GameEntity>> currentSet(sceneEntities.begin(), sceneEntities.end());
	std::set<ref<GameEntity>> existingSet;
	for (auto& gpuData : m_entityGPUData)
		existingSet.insert(gpuData.gameEntity);

	// Check for any difference
	/*if (currentSet == existingSet)
		return;*/

	// Find removed entities
	std::vector<ref<GameEntity>> removed;
	for (auto& e : existingSet)
		if (currentSet.find(e) == currentSet.end())
			removed.push_back(e);

	// Find added entities
	std::vector<ref<GameEntity>> added;
	for (auto& e : currentSet)
		if (existingSet.find(e) == existingSet.end())
			added.push_back(e);

	/*if (removed.empty() && added.empty())
		return;*/

	// Remove stale GPU data
	for (auto& entity : removed) {
		m_entityGPUData.erase(
			std::remove_if(m_entityGPUData.begin(), m_entityGPUData.end(),
				[&entity](const DX12EntityGPUData& d) { return d.gameEntity == entity; }),
			m_entityGPUData.end());
	}

	// Upload and register newly added entities
	/*if (!added.empty()) {
		UploadTexturesAndMeshes(m_scene);
		InitializeEntityGPUData(added);
	}*/
	UploadTexturesAndMeshes(m_scene);
	InitializeEntityGPUData(added);
	// Rebuild pipelines for the full updated entity list
	InitializePipelines(m_overrideMaterial);
}

ref<LuxonEngine::Mesh> LuxonEngine::Rendering::DX12::DX12EditorGraphicContext::ExtractMeshFromGameEntity(const ref<GameEntity>& entity)
{
	auto renderer = entity->GetRenderer();

	if(renderer != nullptr) {
		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(renderer);
		if (meshRenderer != nullptr)
			return meshRenderer->GetMesh();

		auto gbufferRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(renderer);
		if (gbufferRenderer != nullptr)
			return gbufferRenderer->GetMesh();
	}

	auto rtComponent = entity->GetRayTracingComponent();

	if (rtComponent != nullptr)
		return rtComponent->GetMesh();

	return nullptr;
}
