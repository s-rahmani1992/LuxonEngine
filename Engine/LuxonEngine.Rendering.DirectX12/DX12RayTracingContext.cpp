#include "pch.h"
#include "DX12RayTracingContext.h"
#include "Core/GameEntity.h"
#include "Rendering/RayTracingComponent.h"
#include "DX12MeshController.h"
#include "Core/Mesh.h"
#include "HLSLShaderProgram.h"
#include "DX12Utilities.h"
#include "RayTracing/DX12RayTracingPipelineModule.h"
#include "Platform/GraphicWindow.h"
#include "DX12CommandExecuter.h"
#include "Core/Scene.h"
#include "DX12LightManager.h"

bool QuantumEngine::Rendering::DX12::DX12RayTracingContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	if (InitializeSwapChain(factory) == false)
		return false;

	return true;
}

bool QuantumEngine::Rendering::DX12::DX12RayTracingContext::PrepareScene(const ref<Scene>& scene)
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

void QuantumEngine::Rendering::DX12::DX12RayTracingContext::Render()
{
	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	m_rayTracingPipeline->RenderCommand(m_commandList, m_camera);

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

	D3D12_RESOURCE_BARRIER outputRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_rayTracingPipeline->GetOutputBuffer().Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COMMON,
		.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
		},
	};

	m_commandList->ResourceBarrier(1, &outputRTBarrier);
	m_commandList->CopyResource(m_renderBuffers[m_current_buffer_index].Get(), m_rayTracingPipeline->GetOutputBuffer().Get());

	//draw

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

	D3D12_RESOURCE_BARRIER outputEndRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_rayTracingPipeline->GetOutputBuffer().Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
		.StateAfter = D3D12_RESOURCE_STATE_COMMON,
		},
	};
	m_commandList->ResourceBarrier(1, &outputEndRTBarrier);
	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

void QuantumEngine::Rendering::DX12::DX12RayTracingContext::PrepareRayTracingPipeline(const ref<Material>& rtGlobalMaterial)
{
	for (auto& entity : m_entityGPUData) {
		auto rtComponent = entity.gameEntity->GetRayTracingComponent();

		if (rtComponent == nullptr)
			continue;

		m_rtEntityData.push_back(DX12RayTracingGPUData{
			.meshController = std::dynamic_pointer_cast<DX12MeshController>(rtComponent->GetMesh()->GetGPUHandle()),
			.material = rtComponent->GetRTMaterial(),
			.transformResource = entity.transformResource,
			.transform = entity.gameEntity->GetTransform(),
			});
	}

	m_rayTracingPipeline = std::make_shared<RayTracing::DX12RayTracingPipelineModule>();

	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	m_rayTracingPipeline->Initialize(m_commandList, m_rtEntityData, m_window->GetWidth(), m_window->GetHeight(), rtGlobalMaterial, m_cameraBuffer, m_lightManager.GetResource());

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
}
