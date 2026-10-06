#include "pch.h"
#include "DX12HybridContext.h"
#include "Platform/GraphicWindow.h"
#include "DX12Utilities.h"
#include "DX12CommandExecuter.h"
#include "RayTracing/Dx12RayTracingPipelineModule.h"
#include "HLSLShaderProgram.h"
#include <set>
#include <map>
#include "Core/GameEntity.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include <Rendering/ShaderRegistery.h>
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
#include <Rendering/SpikeMeshRenderer.h>
#include <Rendering/SurfaceInstanceRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include "DX12Texture2DController.h"
#include "DX12AssetManager.h"
#include "DX12PipelineFactory.h"
#include <Mesh/DX12MeshPipelineModule.h>
#include "Rasterization/DX12RasterizationPipelineModule.h"
#include <Core/Texture2D.h>
#include <Core/Transform.h>
#include <Core/Logger.h>

bool LuxonEngine::Rendering::DX12::DX12HybridContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	if (InitializeSwapChain(factory) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	return true;
}

bool LuxonEngine::Rendering::DX12::DX12HybridContext::PrepareScene(const ref<Scene>& scene)
{
	if (InitializeCamera(scene->mainCamera) == false)
		return false;

	if (InitializeLight(scene->lightData) == false)
		return false;

	auto colorArray = scene->hybridBackgroundColor.GetColorArray();
	m_hybridBackgroundColor[0] = colorArray[0];
	m_hybridBackgroundColor[1] = colorArray[1];
	m_hybridBackgroundColor[2] = colorArray[2];
	m_hybridBackgroundColor[3] = colorArray[3];

	UploadTexturesAndMeshes(scene);
	InitializeEntityGPUData(scene->entities);
	InitializePipelines();

	return true;
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::Render()
{
	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	m_commandList->SetDescriptorHeaps(1, m_rasterHeap.GetAddressOf());

	if (m_gBufferEntities.size() > 0) {
		RenderGBuffer();
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

	auto dsvHandle = m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->ClearRenderTargetView(m_rtvHandles[m_current_buffer_index], m_hybridBackgroundColor, 0, nullptr);
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
	for(auto& splinePipeline : m_splinePipelines) {
		splinePipeline->Render(m_commandList, m_cameraHandle, m_lightHandle);
	}

	// the pipelines below bind their own descriptor heaps, so they are drawn after the pipelines using the raster heap
	for (auto& pipeline : m_rasterizationModules)
		pipeline->Draw(m_commandList.Get());

	DispatchMeshShadingPipelines(m_meshShadingPipelines);

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

bool LuxonEngine::Rendering::DX12::DX12HybridContext::InitializeDepthBuffer()
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

void LuxonEngine::Rendering::DX12::DX12HybridContext::InitializePipelines()
{
	UInt32 rasterHeapSize = m_entityGPUData.size() + 1 + 1 + 1; // entity transforms + camera + light + gbuffer output?
	std::map<ref<Material>, ref<Rasterization::DX12RasterizationMaterial>> usedMaterials;

	for (auto& entityGpu : m_entityGPUData) {
		// spike and surface instance renderers use mesh shading materials, and the mesh and g buffer renderers are drawn by the rasterization pipelines. their pipelines are created separately
		if (std::dynamic_pointer_cast<SpikeMeshRenderer>(entityGpu.gameEntity->GetRenderer()) != nullptr ||
			std::dynamic_pointer_cast<SurfaceInstanceRenderer>(entityGpu.gameEntity->GetRenderer()) != nullptr ||
			std::dynamic_pointer_cast<MeshRenderer>(entityGpu.gameEntity->GetRenderer()) != nullptr ||
			std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(entityGpu.gameEntity->GetRenderer()) != nullptr)
			continue;

		auto material = entityGpu.gameEntity->GetRenderer()->GetMaterial();

		if (usedMaterials.emplace(material, nullptr).second == false)
			continue;
		
		auto program = std::dynamic_pointer_cast<LuxonEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram>(material->GetProgram());
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

		auto gBufferMeshRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(entityGpu.gameEntity->GetRenderer());
		if (gBufferMeshRenderer != nullptr) {
			m_gBufferEntities.push_back(EntityGBufferData{
				.renderer = gBufferMeshRenderer,
				.entity = entityGpu.gameEntity.get(),
				.transformCpuHandle = entityGpu.transformHeap->GetCPUDescriptorHandleForHeapStart(),
				});
		}

		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(entityGpu.gameEntity->GetRenderer());
		if(splineRenderer != nullptr)
		{
			ref<DX12SplineRasterPipelineModule> splinePipeline = std::make_shared<DX12SplineRasterPipelineModule>(SplineRendererData{
				.renderer = splineRenderer,
				.material = usedMaterials[splineRenderer->GetMaterial()],
				.transformHandle = gpuHandle
				}, m_depthFormat, std::dynamic_pointer_cast<Compute::HLSLComputeProgram>(GetInternalProgram("Bezier_Curve_Compute_Program")));
			splinePipeline->Initialize(m_device);
			splineRenderer->SetDirty();
			m_splinePipelines.push_back(splinePipeline);
		}

		firstHandle.ptr += incrementSize;
		gpuHandle.ptr += incrementSize;
	}

	if (m_gBufferEntities.size() > 0 && InitializeGBuffer() == false) {
		Logger::LogError("Failed to initialize the g buffer pipeline, the g buffer renderers are not drawn");
		m_gBufferRasterization = nullptr;
		m_gBufferEntities.clear();
	}

	if (m_gBufferEntities.size() > 0) {
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
		auto gBufferRTMaterial = DX12MaterialFactory::BuildMaterial(GetInternalProgram("G_Buffer_RT_Global_Program"));
		gBufferRTMaterial->SetValue("missColor", m_hybridBackgroundColor, sizeof(m_hybridBackgroundColor));
		m_GBufferrayTracingPipeline = std::make_shared<RayTracing::DX12RayTracingPipelineModule>();

		m_commandAllocator->Reset();
		m_commandList->Reset(m_commandAllocator.Get(), nullptr);
		
		m_GBufferrayTracingPipeline->Initialize(m_commandList, rtEntityData, m_window->GetWidth(), m_window->GetHeight(), gBufferRTMaterial, m_cameraBuffer, m_lightManager.GetResource());

		m_commandExecuter->ExecuteAndWait(m_commandList.Get());

		auto rtGlob = m_GBufferrayTracingPipeline->GetMaterialInterface();
		rtGlob->SetDescriptorHandle("_PositionTexture", m_gBuffer.srvHeaps[GBufferResources::Position]->GetGPUDescriptorHandleForHeapStart());
		rtGlob->SetDescriptorHandle("_NormalTexture", m_gBuffer.srvHeaps[GBufferResources::Normal]->GetGPUDescriptorHandleForHeapStart());
		rtGlob->SetDescriptorHandle("_MaskTexture", m_gBuffer.srvHeaps[GBufferResources::Mask]->GetGPUDescriptorHandleForHeapStart());
		
		// the output of the ray tracing stage is read by the third stage. a non shader visible view is the source of the descriptor copy
		D3D12_DESCRIPTOR_HEAP_DESC rtOutputHeapDesc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0,
		};

		if (SUCCEEDED(m_device->CreateDescriptorHeap(&rtOutputHeapDesc, IID_PPV_ARGS(&m_rtOutputCpuHeap)))) {
			D3D12_SHADER_RESOURCE_VIEW_DESC outRTDesc = {};
			outRTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			outRTDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			outRTDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			outRTDesc.Texture2D = D3D12_TEX2D_SRV{ .MipLevels = 1, };
			m_device->CreateShaderResourceView(m_GBufferrayTracingPipeline->GetOutputBuffer().Get(), &outRTDesc, m_rtOutputCpuHeap->GetCPUDescriptorHandleForHeapStart());
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

	m_rasterizationModules = CreateRasterizationPipelines();

	m_meshShadingPipelines = CreateSpikeMeshPipelines(*m_pipelineFactory);

	auto surfaceInstancePipelines = CreateSurfaceInstancePipelines();
	m_meshShadingPipelines.insert(m_meshShadingPipelines.end(), surfaceInstancePipelines.begin(), surfaceInstancePipelines.end());
}

bool LuxonEngine::Rendering::DX12::DX12HybridContext::InitializeGBuffer()
{
	m_gBufferMaterial = DX12MaterialFactory::BuildMaterial(GetInternalProgram("G_Buffer_Program"));

	if (m_gBufferMaterial == nullptr)
		return false;

	m_gBuffer = GBufferResources{};
	auto& g = m_gBuffer;

	D3D12_RESOURCE_DESC bufferDesc{
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width = m_window->GetWidth(),
		.Height = m_window->GetHeight(),
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
	};

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = GBufferResources::TargetCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};

	// a shader visible heap can not be the source of a descriptor copy, so the views exist in a CPU only heap too
	D3D12_DESCRIPTOR_HEAP_DESC cpuSrvHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = GBufferResources::TargetCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0,
	};

	if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g.rtvHeap))) ||
		FAILED(m_device->CreateDescriptorHeap(&cpuSrvHeapDesc, IID_PPV_ARGS(&g.cpuSrvHeap))))
		return false;

	auto rtvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto srvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto rtvStart = g.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	auto cpuSrvStart = g.cpuSrvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UInt32 i = 0; i < GBufferResources::TargetCount; i++) {
		bufferDesc.Format = g.formats[i];

		D3D12_CLEAR_VALUE clearValue{
			.Format = g.formats[i],
			.Color = { 0.0f, 0.0f, 0.0f, 0.0f }
		};

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&g.buffers[i]))))
			return false;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{
			.Format = g.formats[i],
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = D3D12_TEX2D_RTV{ .MipSlice = 0, .PlaneSlice = 0 },
		};

		g.rtvHandles[i] = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = rtvStart.ptr + i * rtvIncrementSize };
		m_device->CreateRenderTargetView(g.buffers[i].Get(), &rtvDesc, g.rtvHandles[i]);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format = g.formats[i],
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = D3D12_TEX2D_SRV{ .MipLevels = 1 },
		};

		if (FAILED(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g.srvHeaps[i]))))
			return false;

		m_device->CreateShaderResourceView(g.buffers[i].Get(), &srvDesc, g.srvHeaps[i]->GetCPUDescriptorHandleForHeapStart());

		g.cpuSrvHandles[i] = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = cpuSrvStart.ptr + i * srvIncrementSize };
		m_device->CreateShaderResourceView(g.buffers[i].Get(), &srvDesc, g.cpuSrvHandles[i]);
	}

	// Depth buffer of the g buffer pass
	D3D12_RESOURCE_DESC depthResourceDesc{
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Alignment = 0,
		.Width = m_window->GetWidth(),
		.Height = m_window->GetHeight(),
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = m_depthFormat,
		.SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
	};

	D3D12_CLEAR_VALUE depthClearValue{
		.Format = m_depthFormat,
		.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ .Depth = 1.0f, .Stencil = 0 },
	};

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&g.depthBuffer))))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};

	if (FAILED(m_device->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&g.depthHeap))))
		return false;

	D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc{
		.Format = m_depthFormat,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D = D3D12_TEX2D_DSV{ .MipSlice = 0 },
	};

	m_device->CreateDepthStencilView(g.depthBuffer.Get(), &depthViewDesc, g.depthHeap->GetCPUDescriptorHandleForHeapStart());

	// the rasterization pipeline that draws into the g buffer. the formats of the pixel shader reflection do not describe the g buffer targets
	Rasterization::RasterizationPipelineProperties properties;
	properties.renderTargetFormats.assign(std::begin(g.formats), std::end(g.formats));

	std::string error;
	m_gBufferRasterization = m_pipelineFactory->CreateRasterizationPipeline(m_gBufferMaterial.get(), properties, error);

	if (m_gBufferRasterization == nullptr) {
		Logger::LogError("Failed to create the g buffer pipeline: " + error);
		return false;
	}

	if (m_gBufferRasterization->Initialize((UInt32)m_gBufferEntities.size()) == false)
		return false;

	// the camera view of the base context is in a non shader visible heap, so it can be the source of a descriptor copy
	m_gBufferRasterization->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());

	for (auto& entityData : m_gBufferEntities) {
		auto meshController = std::dynamic_pointer_cast<DX12MeshController>(entityData.renderer->GetMesh()->GetGPUHandle());

		if (meshController == nullptr) // the mesh is not uploaded to the GPU, the entity is not drawn
			continue;

		m_gBufferRasterization->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformCpuHandle);
		m_gBufferRasterization->SetEntityGeometry(entityData.entity, *meshController->GetVertexView(), *meshController->GetIndexView(), meshController->GetMesh()->GetIndexCount());
	}

	return true;
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::RenderGBuffer()
{
	auto& g = m_gBuffer;

	D3D12_RESOURCE_BARRIER barriers[GBufferResources::TargetCount];

	for (UInt32 i = 0; i < GBufferResources::TargetCount; i++) {
		barriers[i] = D3D12_RESOURCE_BARRIER{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
				.pResource = g.buffers[i].Get(),
				.Subresource = 0,
				.StateBefore = D3D12_RESOURCE_STATE_COMMON,
				.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
			},
		};
	}

	m_commandList->ResourceBarrier(GBufferResources::TargetCount, barriers);

	auto dsvHandle = g.depthHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	float clearPositionAndNormal[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float clearMask = 0.0f;
	m_commandList->ClearRenderTargetView(g.rtvHandles[GBufferResources::Position], clearPositionAndNormal, 0, nullptr);
	m_commandList->ClearRenderTargetView(g.rtvHandles[GBufferResources::Normal], clearPositionAndNormal, 0, nullptr);
	m_commandList->ClearRenderTargetView(g.rtvHandles[GBufferResources::Mask], &clearMask, 0, nullptr);
	m_commandList->OMSetRenderTargets(GBufferResources::TargetCount, g.rtvHandles, false, &dsvHandle);

	D3D12_VIEWPORT viewPort{};
	viewPort.Height = m_window->GetHeight();
	viewPort.Width = m_window->GetWidth();
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewPort);

	RECT scissorRect{};
	scissorRect.left = scissorRect.top = 0;
	scissorRect.right = m_window->GetWidth();
	scissorRect.bottom = m_window->GetHeight();
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// the pipeline binds its own descriptor heap
	m_gBufferRasterization->Draw(m_commandList.Get());

	for (auto& barrier : barriers)
		std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);

	m_commandList->ResourceBarrier(GBufferResources::TargetCount, barriers);
}

std::vector<ref<LuxonEngine::Rendering::DX12::Rasterization::DX12RasterizationPipelineModule>> LuxonEngine::Rendering::DX12::DX12HybridContext::CreateRasterizationPipelines()
{
	std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> pipelines;

	struct RasterizationEntityData {
		GameEntity* entity;
		D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
		ref<DX12MeshController> meshController;
	};

	std::map<ref<Material>, std::vector<RasterizationEntityData>> materialMap;

	// group the entities by their material, one pipeline is created per material
	for (auto& entityGpu : m_entityGPUData) {
		auto renderer = entityGpu.gameEntity->GetRenderer();
		ref<Mesh> mesh;
		ref<Material> material;
		const char* rendererName = nullptr;

		if (auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(renderer)) {
			mesh = meshRenderer->GetMesh();
			material = meshRenderer->GetMaterial();
			rendererName = "mesh renderer";
		}
		else if (auto gBufferRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(renderer)) {
			// the third stage of the reflection renderer. the g buffer and the ray tracing output are bound to the pipeline below
			mesh = gBufferRenderer->GetMesh();
			material = gBufferRenderer->GetMaterial();
			rendererName = "g buffer renderer";
		}

		if (mesh == nullptr)
			continue;

		if (material == nullptr) {
			Logger::LogWarning(std::string("The ") + rendererName + " of " + entityGpu.gameEntity->GetName() + " has no material and is not drawn");
			continue;
		}

		auto meshController = std::dynamic_pointer_cast<DX12MeshController>(mesh->GetGPUHandle());
		if (meshController == nullptr) // the mesh is not uploaded to the GPU
			continue;

		materialMap[material].push_back(RasterizationEntityData{
			.entity = entityGpu.gameEntity.get(),
			.transformHandle = entityGpu.transformHeap->GetCPUDescriptorHandleForHeapStart(),
			.meshController = meshController,
			});
	}

	std::string error;
	Rasterization::RasterizationPipelineProperties properties;

	for (auto& [material, entityList] : materialMap) {
		auto pipeline = m_pipelineFactory->CreateRasterizationPipeline(material.get(), properties, error);

		if (pipeline == nullptr) {
			Logger::LogError("Failed to create the rasterization pipeline: " + error);
			continue;
		}

		if (pipeline->Initialize((UInt32)entityList.size()) == false)
			continue;

		pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());
		pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightManager.GetDescriptor()->GetCPUDescriptorHandleForHeapStart());

		// the variables of the reflection renderer stages. a program that does not use them ignores them
		if (m_gBufferRasterization != nullptr) {
			pipeline->SetDescriptor(INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_gBuffer.cpuSrvHandles[GBufferResources::Position]);
			pipeline->SetDescriptor(INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_gBuffer.cpuSrvHandles[GBufferResources::Normal]);
			pipeline->SetDescriptor(INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_gBuffer.cpuSrvHandles[GBufferResources::Mask]);
		}

		if (m_rtOutputCpuHeap != nullptr)
			pipeline->SetDescriptor(INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputCpuHeap->GetCPUDescriptorHandleForHeapStart());

		for (auto& entityData : entityList) {
			pipeline->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformHandle);
			pipeline->SetEntityGeometry(entityData.entity, *entityData.meshController->GetVertexView(),
				*entityData.meshController->GetIndexView(), entityData.meshController->GetMesh()->GetIndexCount());
		}

		pipelines.push_back(pipeline);
	}

	return pipelines;
}

std::vector<ref<LuxonEngine::Rendering::DX12::MeshShading::DX12MeshPipelineModule>> LuxonEngine::Rendering::DX12::DX12HybridContext::CreateSurfaceInstancePipelines()
{
	std::vector<ref<MeshShading::DX12MeshPipelineModule>> pipelines;
	std::string error;

	struct SurfaceInstanceData {
		GameEntity* entity;
		D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
		ref<DX12Texture2DController> maskTexture;
		ref<SurfaceInstanceRenderer> surfaceInstanceRenderer;
	};

	std::map<ref<Material>, std::vector<SurfaceInstanceData>> surfaceInstanceMap;
	std::vector<ref<Mesh>> instanceMeshes;

	// group the surface instance entities by material, one mesh pipeline is created per material
	for (auto& entityGpu : m_entityGPUData) {
		auto surfaceInstanceRenderer = std::dynamic_pointer_cast<SurfaceInstanceRenderer>(entityGpu.gameEntity->GetRenderer());
		if (surfaceInstanceRenderer == nullptr || surfaceInstanceRenderer->GetMaterial() == nullptr || surfaceInstanceRenderer->GetNearMesh() == nullptr ||
			surfaceInstanceRenderer->GetMaskTexture() == nullptr || surfaceInstanceRenderer->GetDensity() <= 0.0f)
			continue;

		// outside the editor the textures are not uploaded on import. already uploaded textures are skipped
		m_assetManager->UploadTextureToGPU(surfaceInstanceRenderer->GetMaskTexture());

		auto maskTexture = std::dynamic_pointer_cast<DX12Texture2DController>(surfaceInstanceRenderer->GetMaskTexture()->GetGPUHandle());
		if (maskTexture == nullptr) // the mask is not uploaded to the GPU
			continue;

		instanceMeshes.push_back(surfaceInstanceRenderer->GetNearMesh());

		if (surfaceInstanceRenderer->GetFarMesh() != nullptr)
			instanceMeshes.push_back(surfaceInstanceRenderer->GetFarMesh());

		surfaceInstanceMap[surfaceInstanceRenderer->GetMaterial()].push_back(SurfaceInstanceData{
			.entity = entityGpu.gameEntity.get(),
			.transformHandle = entityGpu.transformHeap->GetCPUDescriptorHandleForHeapStart(),
			.maskTexture = maskTexture,
			.surfaceInstanceRenderer = surfaceInstanceRenderer,
			});
	}

	// the base mesh of the renderer is null, so the near and far meshes are not uploaded with the other meshes. already uploaded meshes are skipped
	if (instanceMeshes.empty() == false)
		m_assetManager->UploadMeshesToGPU(instanceMeshes);

	// one task group per cell, every dimension of a DispatchMesh is limited to 65535 groups
	constexpr UInt32 maxGroupsPerDimension = 65535;
	MeshShading::MeshPipelineProperties properties;

	for (auto& [material, surfaceInstanceList] : surfaceInstanceMap) {
		ref<MeshShading::DX12MeshPipelineModule> surfaceInstancePipeline = m_pipelineFactory->CreateMeshPipeline(material.get(), properties, error);

		if (surfaceInstancePipeline == nullptr) {
			Logger::LogError("Failed to create the surface instance pipeline: " + error);
			continue;
		}

		if (surfaceInstancePipeline->Initialize((UInt32)surfaceInstanceList.size()) == false)
			continue;

		surfaceInstancePipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());
		surfaceInstancePipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightManager.GetDescriptor()->GetCPUDescriptorHandleForHeapStart());

		for (auto& surfaceInstanceData : surfaceInstanceList) {
			auto& renderer = surfaceInstanceData.surfaceInstanceRenderer;

			// without a far mesh, the near mesh is bound to both levels
			auto farMesh = renderer->GetFarMesh() != nullptr ? renderer->GetFarMesh() : renderer->GetNearMesh();
			auto nearMeshController = std::dynamic_pointer_cast<DX12MeshController>(renderer->GetNearMesh()->GetGPUHandle());
			auto farMeshController = std::dynamic_pointer_cast<DX12MeshController>(farMesh->GetGPUHandle());

			if (nearMeshController == nullptr || farMeshController == nullptr) // the meshes are not uploaded to the GPU, the entity is not drawn
				continue;

			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, surfaceInstanceData.transformHandle);
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_MASK_TEXTURE_NAME, surfaceInstanceData.maskTexture->GetShaderView()->GetCPUDescriptorHandleForHeapStart());
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_NEAR_VERTEX_BUFFER_NAME, nearMeshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_NEAR_INDEX_BUFFER_NAME, nearMeshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_FAR_VERTEX_BUFFER_NAME, farMeshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
			surfaceInstancePipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_FAR_INDEX_BUFFER_NAME, farMeshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());

			// the plane size comes from the x and z scale of the transform, density is the number of cells per unit length
			Vector3 planeSize = surfaceInstanceData.entity->GetTransform()->Scale();
			float density = renderer->GetDensity();
			UInt32 cellCount[2] = {
				std::max(1u, (UInt32)std::ceil(std::abs(planeSize.x) * density)),
				std::max(1u, (UInt32)std::ceil(std::abs(planeSize.z) * density)),
			};

			if (cellCount[0] > maxGroupsPerDimension || cellCount[1] > maxGroupsPerDimension) {
				Logger::LogWarning("The surface instance grid of " + surfaceInstanceData.entity->GetName() + " is clamped to 65535 cells per axis");
				cellCount[0] = std::min(cellCount[0], maxGroupsPerDimension);
				cellCount[1] = std::min(cellCount[1], maxGroupsPerDimension);
			}

			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_step", 1.0f / density);
			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_totalIndices", cellCount);
			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_entityScale", renderer->GetInstanceScale());
			surfaceInstancePipeline->SetEntityConstant(surfaceInstanceData.entity, "_distance", renderer->GetNearDistance());
			surfaceInstancePipeline->SetEntityThreadGroupCount(surfaceInstanceData.entity, cellCount[0], cellCount[1]);
		}

		pipelines.push_back(surfaceInstancePipeline);
	}

	return pipelines;
}


ref<LuxonEngine::Rendering::ShaderProgram> LuxonEngine::Rendering::DX12::DX12HybridContext::GetInternalProgram(const std::string& identifier) const
{
	if (m_shaderProgramRegistery == nullptr)
		return nullptr;

	ShaderProgram* program = m_shaderProgramRegistery->GetShaderProgram(identifier);
	if (program == nullptr)
		return nullptr;

	// the registery owns the program. the empty deleter keeps the ref from deleting it
	return ref<ShaderProgram>(program, [](ShaderProgram*) {});
}
