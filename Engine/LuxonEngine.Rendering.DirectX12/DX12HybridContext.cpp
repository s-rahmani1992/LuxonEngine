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
#include <Rendering/SplineRenderer.h>
#include "DX12SplineVertex.h"
#include "Compute/DX12ComputePipelineModule.h"
#include "DX12MaterialFactory.h"
#include "RayTracing/DX12RayTracingMaterial.h"
#include <Rendering/SpikeMeshRenderer.h>
#include <Rendering/SurfaceInstanceRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include "DX12Texture2DController.h"
#include "DX12AssetManager.h"
#include "DX12PipelineFactory.h"
#include "Core/DX12GPUResourceManager.h"
#include <Mesh/DX12MeshPipelineModule.h>
#include "Rasterization/DX12RasterizationPipelineModule.h"
#include <Core/Texture2D.h>
#include <Core/Transform.h>
#include <Core/Logger.h>
#include "DX12Buffer.h"

bool LuxonEngine::Rendering::DX12::DX12HybridContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	m_resourceManager = std::make_shared<DX12GPUResourceManager>(device.Get(), m_bufferCount);

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

	if (InitializeMainRenderPass() == false)
		return false;

	UploadTexturesAndMeshes(scene);
	InitializeEntityGPUData(scene->entities);
	InitializePipelines();

	return true;
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::Render()
{
	m_resourceManager->BeginFrame();
	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	if (m_gBufferEntities.size() > 0 && m_gBufferRenderPass != nullptr) {
		m_gBufferRenderPass->Execute(m_commandList.Get());
		m_GBufferrayTracingPipeline->RenderCommand(m_commandList, m_camera);
	}

	UpdateSplines();

	if (m_mainRenderPass != nullptr)
		m_mainRenderPass->Execute(m_commandList.Get());

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

bool LuxonEngine::Rendering::DX12::DX12HybridContext::InitializeDepthBuffer()
{
	m_depthTexture = m_resourceManager->CreateDepthTexture(DX12TextureDesc{
		.width = m_window->GetWidth(),
		.height = m_window->GetHeight(),
		.format = m_depthFormat,
		.initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
		.clearValue = D3D12_CLEAR_VALUE{
			.Format = m_depthFormat,
			.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ .Depth = 1.0f, .Stencil = 0 } },
		.name = L"Main Depth Texture",
		});

	return m_depthTexture != nullptr;
}

bool LuxonEngine::Rendering::DX12::DX12HybridContext::InitializeMainRenderPass()
{
	m_mainRenderPass = std::make_unique<DX12RenderPass>();

	DX12RenderPassParams params;
	params.depthTarget = m_depthTexture.get();
	params.clearColors = { { m_hybridBackgroundColor[0], m_hybridBackgroundColor[1], m_hybridBackgroundColor[2], m_hybridBackgroundColor[3] } };

	if (m_mainRenderPass->Initialize(m_swapChain.Get(), params) == false) {
		Logger::LogError("Failed to initialize the main render pass");
		m_mainRenderPass = nullptr;
		return false;
	}

	return true;
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::InitializePipelines()
{
	for (auto& entityGpu : m_entityGPUData) {
		auto gBufferRenderer = std::dynamic_pointer_cast<GBufferRTReflectionRenderer>(entityGpu.gameEntity->GetRenderer());

		if (gBufferRenderer == nullptr)
			continue;

		m_gBufferEntities.push_back(EntityGBufferData{
			.renderer = gBufferRenderer,
			.entity = entityGpu.gameEntity.get(),
			.transformCpuHandle = entityGpu.transformHeap->GetCPUDescriptorHandleForHeapStart(),
			});
	}

	if (m_gBufferEntities.size() > 0 && InitializeGBuffer() == false) {
		Logger::LogError("Failed to initialize the g buffer pipeline, the g buffer renderers are not drawn");
		m_gBufferRasterization = nullptr;
		m_gBufferRenderPass = nullptr;
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
		rtGlob->SetCPUDescriptor("_PositionTexture", m_gBuffer.renderTextures[GBufferResources::Position]->GetSrvHandle());
		rtGlob->SetCPUDescriptor("_NormalTexture", m_gBuffer.renderTextures[GBufferResources::Normal]->GetSrvHandle());
		rtGlob->SetCPUDescriptor("_MaskTexture", m_gBuffer.renderTextures[GBufferResources::Mask]->GetSrvHandle());
		
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

	CreateRasterizationPipelines();
	CreateSplinePipelines();

	// created by the shared base class setup, so they are added to the pass here. the pass keeps the modules alive
	for (auto& spikePipeline : CreateSpikeMeshPipelines(*m_pipelineFactory))
		m_mainRenderPass->AddPipeline(spikePipeline);

	// added to the pass by their own setup
	CreateSurfaceInstancePipelines();
}

bool LuxonEngine::Rendering::DX12::DX12HybridContext::InitializeGBuffer()
{
	m_gBufferMaterial = DX12MaterialFactory::BuildMaterial(GetInternalProgram("G_Buffer_Program"));

	if (m_gBufferMaterial == nullptr)
		return false;

	m_gBufferRenderPass = nullptr; // it points to the textures that are recreated below
	m_gBuffer = GBufferResources{};
	auto& g = m_gBuffer;

	for (int i = 0; i < m_gBuffer.TargetCount; i++) {
		m_gBuffer.renderTextures[i] = m_resourceManager->CreateRenderTexture(DX12TextureDesc{
			.width = m_window->GetWidth(),
			.height = m_window->GetHeight(),
			.format = g.formats[i],
			.clearValue = D3D12_CLEAR_VALUE{ 
				.Format = g.formats[i],
				.Color = { g.clearColors[i][0], g.clearColors[i][1], g.clearColors[i][2], g.clearColors[i][3] } },
			});

		if (m_gBuffer.renderTextures[i] == nullptr)
			return false;

		m_gBuffer.rtvHandles[i] = m_gBuffer.renderTextures[i]->GetRtvHandle();
	}

	m_gBuffer.depthTexture = m_resourceManager->CreateDepthTexture(DX12TextureDesc{
		.width = m_window->GetWidth(),
		.height = m_window->GetHeight(),
		.format = m_depthFormat,
		.clearValue = D3D12_CLEAR_VALUE{
			.Format = m_depthFormat,
			.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ .Depth = 1.0f, .Stencil = 0 } },
		});

	if (g.depthTexture == nullptr)
		return false;

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

	// phase 1: the g buffer renderers are drawn into the g buffer textures
	std::vector<DX12RenderTexture*> gBufferTargets;
	DX12RenderPassParams gBufferPassParams;
	gBufferPassParams.depthTarget = g.depthTexture.get();
	gBufferPassParams.renderTargetFinalState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	for (UInt32 i = 0; i < GBufferResources::TargetCount; i++) {
		gBufferTargets.push_back(g.renderTextures[i].get());
		gBufferPassParams.clearColors.push_back({ g.clearColors[i][0], g.clearColors[i][1], g.clearColors[i][2], g.clearColors[i][3] });
	}

	m_gBufferRenderPass = std::make_unique<DX12RenderPass>();
	if (m_gBufferRenderPass->Initialize(gBufferTargets, gBufferPassParams) == false) {
		m_gBufferRenderPass = nullptr;
		return false;
	}
	m_gBufferRenderPass->AddPipeline(m_gBufferRasterization);

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

void LuxonEngine::Rendering::DX12::DX12HybridContext::CreateRasterizationPipelines()
{
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
			pipeline->SetDescriptor(INTERNAL_GBUFFER_POSITION_TEXTURE_NAME, m_gBuffer.renderTextures[GBufferResources::Position]->GetSrvHandle());
			pipeline->SetDescriptor(INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME, m_gBuffer.renderTextures[GBufferResources::Normal]->GetSrvHandle());
			pipeline->SetDescriptor(INTERNAL_GBUFFER_MASK_TEXTURE_NAME, m_gBuffer.renderTextures[GBufferResources::Mask]->GetSrvHandle());
		}

		if (m_rtOutputCpuHeap != nullptr)
			pipeline->SetDescriptor(INTERNAL_RT_OUTPUT_TEXTURE_NAME, m_rtOutputCpuHeap->GetCPUDescriptorHandleForHeapStart());

		for (auto& entityData : entityList) {
			pipeline->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformHandle);
			pipeline->SetEntityGeometry(entityData.entity, *entityData.meshController->GetVertexView(),
				*entityData.meshController->GetIndexView(), entityData.meshController->GetMesh()->GetIndexCount());
		}

		m_mainRenderPass->AddPipeline(pipeline);
	}
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
		m_mainRenderPass->AddPipeline(surfaceInstancePipeline);
	}

	return pipelines;
}


void LuxonEngine::Rendering::DX12::DX12HybridContext::CreateSplinePipelines()
{
	m_splines.clear();

	struct SplineEntry {
		GameEntity* entity;
		D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
		ref<SplineRenderer> renderer;
	};

	std::vector<SplineEntry> splineEntries;

	for (auto& entityGpu : m_entityGPUData) {
		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(entityGpu.gameEntity->GetRenderer());
		if (splineRenderer == nullptr || splineRenderer->GetSegments() <= 0)
			continue;

		if (splineRenderer->GetMaterial() == nullptr) {
			Logger::LogWarning("The spline renderer of " + entityGpu.gameEntity->GetName() + " has no material and is not drawn");
			continue;
		}

		splineEntries.push_back(SplineEntry{
			.entity = entityGpu.gameEntity.get(),
			.transformHandle = entityGpu.transformHeap->GetCPUDescriptorHandleForHeapStart(),
			.renderer = splineRenderer,
			});
	}

	if (splineEntries.empty())
		return;

	auto computeProgram = GetInternalProgram("Bezier_Curve_Compute_Program");

	if (computeProgram == nullptr) {
		Logger::LogError("The spline compute program is not registered, the splines are not drawn");
		return;
	}

	// the unordered access views of the vertex buffers are the source of the descriptor copies of the compute modules
	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = (UInt32)splineEntries.size(),
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};

	auto incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// the thread group size of the compute shader
	constexpr UInt32 threadsPerGroup = 32;
	std::string error;
	std::map<ref<Material>, std::vector<size_t>> materialMap; // material of the spline renderer -> indices in m_splines

	for (UInt32 i = 0; i < splineEntries.size(); i++) {
		auto& entry = splineEntries[i];
		UInt32 vertexCount = (UInt32)entry.renderer->GetSegments() + 1;

		SplineGPUData spline;
		spline.renderer = entry.renderer;
		spline.entity = entry.entity;
		spline.transformHandle = entry.transformHandle;

		spline.vertexBufferData = m_resourceManager->CreateStructuredBuffer<SplineVertex>(DX12BufferDesc{
			.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			}, vertexCount);

		if (!spline.vertexBufferData) {
			Logger::LogError("Failed to create the vertex buffer of " + entry.entity->GetName());
			continue;
		}
		// every spline has its own compute material, because the curve parameters are material values
		spline.computeMaterial = DX12MaterialFactory::BuildMaterial(computeProgram);
		spline.computeModule = spline.computeMaterial != nullptr ? m_pipelineFactory->CreateComputePipeline(spline.computeMaterial.get(), error) : nullptr;

		if (spline.computeModule == nullptr) {
			Logger::LogError("Failed to create the spline compute pipeline: " + error);
			continue;
		}

		if (spline.computeModule->Initialize(1) == false)
			continue;

		spline.computeModule->SetEntityDescriptor(spline.entity, INTERNAL_VERTEX_BUFFER_NAME, spline.vertexBufferData->GetWriteHandle());
		spline.computeModule->SetEntityThreadGroupCount(spline.entity, (vertexCount + threadsPerGroup - 1) / threadsPerGroup);

		// the vertices are generated on the first render
		spline.renderer->SetDirty();

		materialMap[spline.renderer->GetMaterial()].push_back(m_splines.size());
		m_splines.push_back(std::move(spline));
	}

	// the geometry shader turns every line of the strip into a quad, so the quads do not have a fixed winding
	Rasterization::RasterizationPipelineProperties properties;
	properties.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	properties.stripTopology = true;
	properties.cullMode = D3D12_CULL_MODE_NONE;

	for (auto& [material, splineIndices] : materialMap) {
		auto pipeline = m_pipelineFactory->CreateRasterizationPipeline(material.get(), properties, error);

		if (pipeline == nullptr) {
			Logger::LogError("Failed to create the spline rasterization pipeline: " + error);
			continue;
		}

		if (pipeline->Initialize((UInt32)splineIndices.size()) == false)
			continue;

		pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());
		pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, m_lightManager.GetDescriptor()->GetCPUDescriptorHandleForHeapStart());

		for (auto splineIndex : splineIndices) {
			auto& spline = m_splines[splineIndex];
			float width = spline.renderer->GetWidth();

			pipeline->SetEntityDescriptor(spline.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, spline.transformHandle);
			pipeline->SetEntityGeometry(spline.entity, spline.vertexBufferData->GetVertexBufferView(), (UInt32)spline.renderer->GetSegments() + 1);
			pipeline->SetEntityConstant(spline.entity, INTERNAL_SPLINE_WIDTH_NAME, width);
			spline.rasterModule = pipeline;
		}

		m_mainRenderPass->AddPipeline(pipeline);
	}
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::UpdateSplines()
{
	for (auto& spline : m_splines) {
		if (spline.renderer->IsDirty() == false)
			continue;

		auto& curve = spline.renderer->GetCurve();

		// the curve parameters are the material values of the compute program
		spline.computeMaterial->SetValue("startPoint", curve.m_point1);
		spline.computeMaterial->SetValue("midPoint", curve.m_point2);
		spline.computeMaterial->SetValue("endPoint", curve.m_point3);
		spline.computeMaterial->SetValue("tileFactor", spline.renderer->GetTileFactor());
		spline.computeMaterial->SetValue("length", curve.InterpolateLength(1.0f));

		if (spline.rasterModule != nullptr) {
			float width = spline.renderer->GetWidth();
			spline.rasterModule->SetEntityConstant(spline.entity, INTERNAL_SPLINE_WIDTH_NAME, width);
		}

		// the vertex buffer is a vertex buffer in the common state and an unordered access buffer while it is generated
		D3D12_RESOURCE_BARRIER barrier{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
				.pResource = spline.vertexBufferData->GetResource(),
				.Subresource = 0,
				.StateBefore = D3D12_RESOURCE_STATE_COMMON,
				.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			},
		};

		m_commandList->ResourceBarrier(1, &barrier);
		spline.computeModule->Dispatch(m_commandList.Get());

		std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
		m_commandList->ResourceBarrier(1, &barrier);
	}
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
