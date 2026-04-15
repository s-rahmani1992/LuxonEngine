#include "pch.h"
#include "DX12HybridContext.h"
#include "Platform/GraphicWindow.h"
#include "DX12Utilities.h"
#include "DX12CommandExecuter.h"
#include "DX12GameEntityPipelineModule.h"
#include "RayTracing/Dx12RayTracingPipelineModule.h"
#include "DX12GBufferPipelineModule.h"
#include "HLSLShaderProgram.h"
#include <set>
#include <map>
#include "Core/GameEntity.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include "DX12ShaderRegistery.h"
#include "Rendering/RayTracingComponent.h"
#include "DX12MeshController.h"
#include "Core/Mesh.h"
#include "Core/Scene.h"
#include "Rasterization/DX12RasterizationMaterial.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include <Rendering/SplineRenderer.h>
#include "DX12SplineRasterPipelineModule.h"
#include "Compute/HLSLComputeProgram.h"
#include "DX12MaterialFactory.h"
#include "RayTracing/DX12RayTracingMaterial.h"

bool QuantumEngine::Rendering::DX12::DX12HybridContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	if (InitializeSwapChain(factory) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	return true;
}

bool QuantumEngine::Rendering::DX12::DX12HybridContext::PrepareScene(const ref<Scene>& scene)
{
	if (InitializeCamera(scene->mainCamera) == false)
		return false;

	if (InitializeLight(scene->lightData) == false)
		return false;

	UploadTexturesAndMeshes(scene);
	InitializeEntityGPUData(scene->entities);
	InitializePipelines();
	return true;
}

void QuantumEngine::Rendering::DX12::DX12HybridContext::Render()
{
	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	m_commandList->SetDescriptorHeaps(1, m_rasterHeap.GetAddressOf());

	if (m_gBufferEntities.size() > 0) {
		m_gBufferPipeline->RenderCommand(m_commandList, m_cameraHandle);
		m_GBufferrayTracingPipeline->RenderCommand(m_commandList, m_camera);
	}

	//Set Render Target
	auto m_current_buffer_index = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER beginBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_renderBuffers[m_current_buffer_index].Get(),
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
	m_commandList->ClearRenderTargetView(m_rtvHandles[m_current_buffer_index], clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_rtvHandles[m_current_buffer_index], false, &dsvHandle);
	//Viewport
	D3D12_VIEWPORT viewPort{};
	viewPort.Height = m_window->GetHeight();
	viewPort.Width = m_window->GetWidth();
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewPort);

	//Rect Scissor
	RECT scissorRect{};
	scissorRect.left = scissorRect.top = 0;
	scissorRect.right = m_window->GetWidth();
	scissorRect.bottom = m_window->GetHeight();
	m_commandList->RSSetScissorRects(1, &scissorRect);

	//draw

	m_commandList->SetDescriptorHeaps(1, m_rasterHeap.GetAddressOf());
	for (auto& pipeline : m_rasterizationPipelines) {
		pipeline->Render(m_commandList, m_cameraHandle, m_lightHandle);
	}

	for(auto& splinePipeline : m_splinePipelines) {
		splinePipeline->Render(m_commandList, m_cameraHandle, m_lightHandle);
	}

	//draw

	D3D12_RESOURCE_BARRIER endBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_renderBuffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
		.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	m_commandList->ResourceBarrier(1, &endBarrier);

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

bool QuantumEngine::Rendering::DX12::DX12HybridContext::InitializeDepthBuffer()
{
	// Create Depth Buffer
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

void QuantumEngine::Rendering::DX12::DX12HybridContext::InitializePipelines()
{
	UInt32 rasterHeapSize = m_entityGPUData.size() + 1 + 1 + 1; // entity transforms + camera + light + gbuffer output?
	std::map<ref<Material>, ref<Rasterization::DX12RasterizationMaterial>> usedMaterials;

	for (auto& entityGpu : m_entityGPUData) {
		auto material = entityGpu.gameEntity->GetRenderer()->GetMaterial();

		if (usedMaterials.emplace(material, nullptr).second == false)
			continue;
		
		auto program = std::dynamic_pointer_cast<QuantumEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram>(material->GetProgram());
		auto rasterMaterial = std::make_shared<Rasterization::DX12RasterizationMaterial>(material, program);
		usedMaterials[material] = rasterMaterial;
		rasterHeapSize += material->GetTextureFieldCount();
	}

	for (auto& entityGpu : m_entityGPUData) {
		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(entityGpu.gameEntity->GetRenderer());
		if (splineRenderer != nullptr) {
			rasterHeapSize += 1;
		}
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = rasterHeapSize,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};

	m_device->CreateDescriptorHeap(&rtHeapDesc, IID_PPV_ARGS(&m_rasterHeap));

	// Create views for camera and lights
	auto incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto firstHandle = m_rasterHeap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandle = m_rasterHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cameraViewDesc;
	cameraViewDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cameraViewDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(CameraGPU));
	m_device->CreateConstantBufferView(&cameraViewDesc, firstHandle);
	m_cameraHandle = gpuHandle;

	firstHandle.ptr += incrementSize;
	gpuHandle.ptr += incrementSize;
	D3D12_CONSTANT_BUFFER_VIEW_DESC lightViewDesc;
	lightViewDesc.BufferLocation = m_lightManager.GetResource()->GetGPUVirtualAddress();
	lightViewDesc.SizeInBytes = m_lightManager.GetResource()->GetDesc().Width;
	m_device->CreateConstantBufferView(&lightViewDesc, firstHandle);
	m_lightHandle = gpuHandle;

	// Populate Transform View and Pipelines
	firstHandle.ptr += incrementSize;
	gpuHandle.ptr += incrementSize;

	for (auto& entityGpu : m_entityGPUData) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC transformViewDesc;
		transformViewDesc.BufferLocation = entityGpu.transformResource->GetGPUVirtualAddress();
		transformViewDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(TransformGPU));
		m_device->CreateConstantBufferView(&transformViewDesc, firstHandle);

		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(entityGpu.gameEntity->GetRenderer());
		if (meshRenderer != nullptr) {
			m_meshRendererData.push_back(DX12MeshRendererGPUData{
				.meshRenderer = meshRenderer,
				.material = usedMaterials[meshRenderer->GetMaterial()],
				.transformResource = entityGpu.transformResource,
				.transformHandle = gpuHandle,
				});
		}

		auto gBufferMeshRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(entityGpu.gameEntity->GetRenderer());
		if (gBufferMeshRenderer != nullptr) {
			m_gBufferEntities.push_back(EntityGBufferData{
				.renderer = gBufferMeshRenderer,
				.transformResource = entityGpu.transformResource,
				.transformHandle = gpuHandle,
				});
		}

		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(entityGpu.gameEntity->GetRenderer());
		if(splineRenderer != nullptr)
		{
			ref<DX12SplineRasterPipelineModule> splinePipeline = std::make_shared<DX12SplineRasterPipelineModule>(SplineRendererData{
				.renderer = splineRenderer,
				.material = usedMaterials[splineRenderer->GetMaterial()],
				.transformHandle = gpuHandle
				}, m_depthFormat, std::dynamic_pointer_cast<Compute::HLSLComputeProgram>(m_shaderRegistery->GetShaderProgram("Bezier_Curve_Compute_Program")));
			splinePipeline->Initialize(m_device);
			m_splinePipelines.push_back(splinePipeline);
		}

		firstHandle.ptr += incrementSize;
		gpuHandle.ptr += incrementSize;
	}

	for (auto& meshRender : m_meshRendererData) {
		auto pipeline = std::make_shared<DX12GameEntityPipelineModule>();
		if (pipeline->Initialize(m_device, meshRender, m_depthFormat) == false) {
			return;
		}

		m_rasterizationPipelines.push_back(pipeline);
	}

	if (m_gBufferEntities.size() > 0) {
		m_gBufferPipeline = std::make_shared<DX12GBufferPipelineModule>();

		if (m_gBufferPipeline->Initialize(m_device, Vector2UInt(m_window->GetWidth(), m_window->GetHeight()), std::dynamic_pointer_cast<Rasterization::HLSLRasterizationProgram>(m_shaderRegistery->GetShaderProgram("G_Buffer_Program"))))
			m_gBufferPipeline->PrepareEntities(m_gBufferEntities);

		// Initialize Ray Tracing Stage of Reflection Renderer
		std::vector<DX12RayTracingGPUData> rtEntityData;

		for (auto& entity : m_entityGPUData) {
			auto rtComponent = entity.gameEntity->GetRayTracingComponent();

			if (rtComponent == nullptr)
				continue;

			rtEntityData.push_back(DX12RayTracingGPUData{
				.meshController = std::dynamic_pointer_cast<DX12MeshController>(rtComponent->GetMesh()->GetGPUHandle()),
				.material = rtComponent->GetRTMaterial(),
				.transformResource = entity.transformResource,
				.transform = entity.gameEntity->GetTransform(),
				});
		}
		auto gBufferRTMaterial = DX12MaterialFactory::BuildMaterial(m_shaderRegistery->GetShaderProgram("G_Buffer_RT_Global_Program"));

		m_GBufferrayTracingPipeline = std::make_shared<RayTracing::DX12RayTracingPipelineModule>();

		m_commandAllocator->Reset();
		m_commandList->Reset(m_commandAllocator.Get(), nullptr);
		
		m_GBufferrayTracingPipeline->Initialize(m_commandList, rtEntityData, m_window->GetWidth(), m_window->GetHeight(), gBufferRTMaterial, m_cameraBuffer, m_lightManager.GetResource());

		m_commandExecuter->ExecuteAndWait(m_commandList.Get());

		auto rtGlob = m_GBufferrayTracingPipeline->GetMaterialInterface();
		rtGlob->SetDescriptorHandle("_PositionTexture", m_gBufferPipeline->GetPositionHeap()->GetGPUDescriptorHandleForHeapStart());
		rtGlob->SetDescriptorHandle("_NormalTexture", m_gBufferPipeline->GetNormalHeap()->GetGPUDescriptorHandleForHeapStart());
		rtGlob->SetDescriptorHandle("_MaskTexture", m_gBufferPipeline->GetMaskHeap()->GetGPUDescriptorHandleForHeapStart());
		
		auto cpuHandle = m_rasterHeap->GetCPUDescriptorHandleForHeapStart();
		auto gpuHandle1 = m_rasterHeap->GetGPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += (rasterHeapSize - 1) * incrementSize;
		gpuHandle1.ptr += (rasterHeapSize - 1) * incrementSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC outRTDesc = {};
		outRTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		outRTDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		outRTDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		outRTDesc.Texture2D = D3D12_TEX2D_SRV{ .MipLevels = 1, };
		m_device->CreateShaderResourceView(m_GBufferrayTracingPipeline->GetOutputBuffer().Get(), &outRTDesc, cpuHandle);

		for (auto& entity : m_gBufferEntities) {
			auto meshRender = std::make_shared<MeshRenderer>(entity.renderer->GetMesh(), entity.renderer->GetMaterial());
			auto pipeline = std::make_shared<DX12GameEntityPipelineModule>();

			auto& material = usedMaterials[entity.renderer->GetMaterial()];
			material->SetDescriptorHandles("_PositionTexture", m_gBufferPipeline->GetPositionHeap()->GetGPUDescriptorHandleForHeapStart());
			material->SetDescriptorHandles("_NormalTexture", m_gBufferPipeline->GetNormalHeap()->GetGPUDescriptorHandleForHeapStart());
			material->SetDescriptorHandles("_MaskTexture", m_gBufferPipeline->GetMaskHeap()->GetGPUDescriptorHandleForHeapStart());
			material->SetDescriptorHandles(HLSL_RT_OUTPUT_TEXTURE_NAME, gpuHandle1);

			if (pipeline->Initialize(m_device, DX12MeshRendererGPUData
				{
					.meshRenderer = meshRender,
					.material = material,
					.transformResource = entity.transformResource,
					.transformHandle = entity.transformHandle
				}, m_depthFormat) == false) {
				continue;
			}

			m_rasterizationPipelines.push_back(pipeline);
		}
	}

	UInt32 offset = 2 + m_entityGPUData.size();
	for (auto& mat : usedMaterials) {
		mat.second->BindDescriptorToResources(m_rasterHeap, offset);
		offset += mat.first->GetTextureFieldCount();
	}

	for(auto& splinePipeline : m_splinePipelines) {
		splinePipeline->BindDescriptorToResources(m_rasterHeap, offset);
		offset += 1;
	}
}

