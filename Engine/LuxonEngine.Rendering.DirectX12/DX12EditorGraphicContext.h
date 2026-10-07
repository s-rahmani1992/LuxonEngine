#pragma once
#include "DX12GraphicContext.h"
#include "DX12HybridContext.h"

namespace LuxonEngine::Rendering::DX12 {

	namespace Rasterization {
		class DX12RasterizationPipelineModule;
	}

	namespace MeshShading {
		class DX12MeshPipelineModule;
	}

	namespace Compute {
		class DX12ComputePipelineModule;
	}

	class DX12EditorGraphicContext : public DX12GraphicContext
	{
	public:
		DX12EditorGraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<LuxonEngine::Platform::GraphicWindow>& window, const ref<DX12AssetManager>& assetManager,
			const ref<DX12PipelineFactory>& pipelineFactory, ShaderRegistery* shaderRegistery)
			: DX12GraphicContext(bufferCount, commandExecuter, window, assetManager), m_pipelineFactory(pipelineFactory), m_shaderProgramRegistery(shaderRegistery) {
		}

		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		bool InitializeDepthBuffer();
		void InitializePipelines();
		void SyncEntities();

		std::vector<ref<MeshShading::DX12MeshPipelineModule>> CreateSurfaceInstancePipelines();

		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> CreateRasterizationPipelines();

		/// <summary>
		/// Creates the pipelines of the spline renderers: a compute module per spline that generates the vertices and one rasterization module per material that draws them.
		/// returns the rasterization modules
		/// </summary>
		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> CreateSplinePipelines();

		/// <summary>
		/// Regenerates the vertices of the splines that changed. it records commands, so it is called while the command list is open
		/// </summary>
		void UpdateSplines();

		ref<ShaderProgram> GetInternalProgram(const std::string& identifier) const;

		static ref<Mesh> ExtractMeshFromGameEntity(const ref<GameEntity>& entity);

	private:
		struct SplineGPUData {
			ref<SplineRenderer> renderer;
			GameEntity* entity = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle = {};
			ComPtr<ID3D12Resource2> vertexBuffer;
			D3D12_VERTEX_BUFFER_VIEW vertexView = {};
			ref<Material> computeMaterial; // holds the curve parameters of the spline
			ref<Compute::DX12ComputePipelineModule> computeModule;
			ref<Rasterization::DX12RasterizationPipelineModule> rasterModule;
		};

		ref<DX12PipelineFactory> m_pipelineFactory;
		ShaderRegistery* m_shaderProgramRegistery;

		// Depth Stencil
		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;
		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_depthStencilvHeap;

		std::vector<ref<MeshShading::DX12MeshPipelineModule>> m_meshShadingPipelines;
		std::vector<ref<Rasterization::DX12RasterizationPipelineModule>> m_rasterizationModules;

		std::vector<SplineGPUData> m_splines;
		ComPtr<ID3D12DescriptorHeap> m_splineUavHeap; // non shader visible, the source of the vertex buffer descriptor copies

		ref<Material> m_overrideMaterial;
		ref<Scene> m_scene;
	};
}