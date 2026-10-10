#include "pch.h"
#include "DX12RayTracingContext.h"
#include "Core/GameEntity.h"
#include "Rendering/RayTracingComponent.h"
#include "DX12MeshController.h"
#include "Core/Mesh.h"
#include "HLSLShaderProgram.h"
#include "DX12Utilities.h"
#include "Platform/GraphicWindow.h"
#include "DX12CommandExecuter.h"
#include "Core/Scene.h"
#include "DX12LightManager.h"
#include "DX12PipelineFactory.h"
#include "Core/DX12GPUResourceManager.h"
#include <Rendering/ShaderInternalNames.h>
#include <Core/Transform.h>
#include "RayTracing/DX12RayTracingPipelineModule.h"
#include <Core/Logger.h>

bool LuxonEngine::Rendering::DX12::DX12RayTracingContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	m_resourceManager = std::make_shared<DX12GPUResourceManager>(device.Get(), m_bufferCount);

	if (InitializeSwapChain(factory) == false)
		return false;

	return true;
}

bool LuxonEngine::Rendering::DX12::DX12RayTracingContext::PrepareScene(const ref<Scene>& scene)
{
	if (InitializeCamera(scene->mainCamera) == false)
		return false;

	if (InitializeLight(scene->lightData) == false)
		return false;

	UploadTexturesAndMeshes(scene);
	InitializeEntityGPUData(scene->entities);
	PrepareRayTracingPipeline(scene->rtGlobalMaterial);
    return true;
}

void LuxonEngine::Rendering::DX12::DX12RayTracingContext::TransitionOutput(D3D12_RESOURCE_STATES state)
{
	if (m_rtOutputTexture->GetState() == state)
		return;

	D3D12_RESOURCE_BARRIER barrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_rtOutputTexture->GetResource(),
		.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		.StateBefore = m_rtOutputTexture->GetState(),
		.StateAfter = state,
		},
	};

	m_commandList->ResourceBarrier(1, &barrier);
	m_rtOutputTexture->SetState(state);
}

void LuxonEngine::Rendering::DX12::DX12RayTracingContext::Render()
{
	if (m_rayTracingPipeline == nullptr || m_rtOutputTexture == nullptr)
		return;

	m_resourceManager->BeginFrame();
	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	TransitionOutput(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_rayTracingPipeline->Dispatch(m_commandList.Get());

	//Set Render Target
	auto m_current_buffer_index = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER copyRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_renderBuffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
		.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
		},
	};

	m_commandList->ResourceBarrier(1, &copyRTBarrier);

	TransitionOutput(D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->CopyResource(m_renderBuffers[m_current_buffer_index].Get(), m_rtOutputTexture->GetResource());

	D3D12_RESOURCE_BARRIER endBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_renderBuffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
		.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	m_commandList->ResourceBarrier(1, &endBarrier);

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

void LuxonEngine::Rendering::DX12::DX12RayTracingContext::PrepareRayTracingPipeline(const ref<Material>& rtGlobalMaterial)
{
	m_rtGlobalMaterial = rtGlobalMaterial;

	// the output of the global program
	m_rtOutputTexture = m_resourceManager->CreateTexture(DX12TextureDesc{
		.width = m_window->GetWidth(),
		.height = m_window->GetHeight(),
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		.name = L"Ray Tracing Output Texture",
		});

	if (m_rtOutputTexture == nullptr) {
		Logger::LogError("Failed to create the output texture of the ray tracing pipeline");
		return;
	}

	// the pipeline is created with all the entities at once
	std::vector<RayTracing::RayTracingEntityDesc> entityDescs;

	for (auto& entity : m_entityGPUData) {
		auto rtComponent = entity.gameEntity->GetRayTracingComponent();

		if (rtComponent != nullptr)
			entityDescs.push_back(RayTracing::RayTracingEntityDesc{ rtComponent.get(), entity.gameEntity.get() });
	}

	std::string error;
	m_rayTracingPipeline = m_pipelineFactory->CreateRayTracingPipeline(RayTracing::RayTracingPipelineProperties{}, m_rtGlobalMaterial.get(), entityDescs, error);

	if (m_rayTracingPipeline == nullptr) {
		Logger::LogError("Failed to create the ray tracing pipeline: " + error);
		return;
	}

	m_rayTracingPipeline->SetDimensions(m_window->GetWidth(), m_window->GetHeight());

	// the views of the base context are in non shader visible heaps, so they can be the source of the descriptor copies
	m_rayTracingPipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());
	m_rayTracingPipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightManager.GetDescriptor()->GetCPUDescriptorHandleForHeapStart());
	m_rayTracingPipeline->SetDescriptor(INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputTexture->GetUavHandle());

	for (auto& entity : m_entityGPUData) {
		auto rtComponent = entity.gameEntity->GetRayTracingComponent();

		if (rtComponent == nullptr || rtComponent->GetMesh() == nullptr)
			continue;

		auto meshController = std::dynamic_pointer_cast<DX12MeshController>(rtComponent->GetMesh()->GetGPUHandle());

		if (meshController == nullptr) // the mesh is not uploaded to the GPU, the entity is not traced
			continue;

		GameEntity* gameEntity = entity.gameEntity.get();
		m_rayTracingPipeline->SetEntityDescriptor(gameEntity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entity.transformHeap->GetCPUDescriptorHandleForHeapStart());
		m_rayTracingPipeline->SetEntityDescriptor(gameEntity, INTERNAL_VERTEX_BUFFER_NAME, meshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
		m_rayTracingPipeline->SetEntityDescriptor(gameEntity, INTERNAL_INDEX_BUFFER_NAME, meshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
	}
}
