#include "pch.h"
#include "DX12HybridContext.h"
#include "Platform/GraphicWindow.h"
#include "DX12CommandExecuter.h"
#include "DX12AssetManager.h"
#include "DX12PipelineFactory.h"
#include "Core/DX12GPUResourceManager.h"
#include "RendererSetup/DX12GBufferRendererSetup.h"
#include "RendererSetup/DX12MeshRendererSetup.h"
#include "RendererSetup/DX12SplineRendererSetup.h"
#include "RendererSetup/DX12SpikeMeshRendererSetup.h"
#include "RendererSetup/DX12SurfaceInstanceRendererSetup.h"
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Rendering/MeshRenderer.h>
#include <Rendering/ShaderRegistery.h>
#include <Rendering/SplineRenderer.h>
#include <Rendering/SpikeMeshRenderer.h>
#include <Rendering/SurfaceInstanceRenderer.h>
#include <Core/GameEntity.h>
#include <Core/Scene.h>
#include <Core/Logger.h>

bool LuxonEngine::Rendering::DX12::DX12HybridContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	if (InitializeCommandObjects(device) == false)
		return false;

	m_resourceManager = std::make_shared<DX12GPUResourceManager>(device.Get(), m_bufferCount);

	m_setupRegistry.Register<GBufferRTReflectionRenderer, DX12GBufferRendererSetup>();
	m_setupRegistry.Register<MeshRenderer, DX12MeshRendererSetup>();
	m_setupRegistry.Register<SplineRenderer, DX12SplineRendererSetup>();
	m_setupRegistry.Register<SpikeMeshRenderer, DX12SpikeMeshRendererSetup>();
	m_setupRegistry.Register<SurfaceInstanceRenderer, DX12SurfaceInstanceRendererSetup>();

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

	return CompilePassSequence();
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::Render()
{
	m_resourceManager->BeginFrame();
	UpdateDataHeaps();

	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	m_setupRegistry.Update();

	m_passSequence.Execute(m_commandList.Get());

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

bool LuxonEngine::Rendering::DX12::DX12HybridContext::CompilePassSequence()
{
	m_passSequence.Clear();
	m_passSequence.SetSwapChain(m_swapChain.Get());

	// which passes run is decided here, the barriers are created for exactly these
	if (m_setupContext.gBufferRenderPass != nullptr)
		m_passSequence.AddPass(m_setupContext.gBufferRenderPass);

	m_passSequence.AddPass(m_computePass.get());
	m_passSequence.AddPass(m_mainRenderPass.get());

	if (m_passSequence.Compile() == false) {
		Logger::LogError("Failed to create the resource barriers of the passes");
		return false;
	}

	return true;
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::RunRendererSetups()
{
	m_setupContext = DX12PassSetupContext();
	m_setupContext.pipelineFactory = m_pipelineFactory.get();
	m_setupContext.resourceManager = m_resourceManager.get();
	m_setupContext.assetManager = m_assetManager.get();
	m_setupContext.getInternalProgram = [this](const std::string& identifier) { return GetInternalProgram(identifier); };
	m_setupContext.renderWidth = m_window->GetWidth();
	m_setupContext.renderHeight = m_window->GetHeight();
	m_setupContext.depthFormat = m_depthFormat;
	m_setupContext.backgroundColor = { m_hybridBackgroundColor[0], m_hybridBackgroundColor[1], m_hybridBackgroundColor[2], m_hybridBackgroundColor[3] };
	m_setupContext.allEntities = std::span<DX12EntityGPUData>(m_entityGPUData);
	m_setupContext.cameraDescriptor = m_cameraHeap->GetCPUDescriptorHandleForHeapStart();
	m_setupContext.lightDescriptor = m_lightManager.GetDescriptor()->GetCPUDescriptorHandleForHeapStart();
	m_setupContext.computePass = m_computePass.get();
	m_setupContext.mainRenderPass = m_mainRenderPass.get();

	std::vector<DX12RendererEntity> rendererEntities;

	for (auto& entityGpu : m_entityGPUData) {
		auto renderer = entityGpu.gameEntity->GetRenderer();

		if (renderer != nullptr)
			rendererEntities.push_back(DX12RendererEntity{ entityGpu.gameEntity.get(), renderer.get(), &entityGpu});
	}

	m_setupRegistry.Setup(rendererEntities, m_setupContext);
}

void LuxonEngine::Rendering::DX12::DX12HybridContext::InitializePipelines()
{
	// the setups release the resources of the previous scene first
	m_setupRegistry.Teardown();

	// the pass points to the resources and the pipelines that the setups create again below, so it is replaced first
	m_computePass = std::make_unique<DX12ComputePass>();

	RunRendererSetups();
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
