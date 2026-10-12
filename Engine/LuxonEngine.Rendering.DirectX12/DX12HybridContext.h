#pragma once
#include "DX12GraphicContext.h"
#include "Core/DX12DepthTexture.h"
#include "Core/DX12RenderPass.h"
#include "Core/DX12ComputePass.h"
#include "Core/DX12PassSequence.h"
#include "Core/DX12RendererSetupRegistry.h"

namespace LuxonEngine::Rendering {
	class ShaderRegistery;
	class ShaderProgram;
}

namespace LuxonEngine::Rendering::DX12 {
	class DX12PipelineFactory;
	class DX12GPUResourceManager;

	class DX12HybridContext : public DX12GraphicContext
	{
	public:
		DX12HybridContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<LuxonEngine::Platform::GraphicWindow>& window, const ref<DX12AssetManager>& assetManager,
			const ref<DX12PipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery)
			: DX12GraphicContext(bufferCount, m_commandExecuter, window, assetManager), m_pipelineFactory(pipelineFactory), m_shaderProgramRegistery(shaderRegistery) {
		}

		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		bool InitializeDepthBuffer();
		bool InitializeMainRenderPass();
		bool CompilePassSequence();
		void InitializePipelines();
		void RunRendererSetups();

		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;

	private:
		ref<DX12PipelineFactory> m_pipelineFactory;
		ref<DX12GPUResourceManager> m_resourceManager;
		ShaderRegistery* m_shaderProgramRegistery;

		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		Ptr<DX12DepthTexture> m_depthTexture;

		float m_hybridBackgroundColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

		DX12RendererSetupRegistry m_setupRegistry;

		DX12PassSetupContext m_setupContext;

		Ptr<DX12ComputePass> m_computePass;
		Ptr<DX12RenderPass> m_mainRenderPass;

		DX12PassSequence m_passSequence;
	};
}
