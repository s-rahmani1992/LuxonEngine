#pragma once
#include "Core/DX12RendererSetup.h"
#include "Core/DX12Buffer.h"

namespace LuxonEngine::Rendering {
	class SplineRenderer;
	class Material;
}

namespace LuxonEngine::Rendering::DX12 {
	namespace Rasterization {
		class DX12RasterizationPipelineModule;
	}

	// Generates the vertices of the splines with a compute pipeline every frame, and draws them with a rasterization pipeline per material.
	class DX12SplineRendererSetup : public IDX12RendererSetup
	{
	public:
		virtual void Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context) override;

		// Copies the curve parameters of the splines that changed into their compute materials.
		virtual void Update() override;

		virtual void Teardown() override;

	private:
		struct SplineGPUData {
			ref<SplineRenderer> renderer;
			GameEntity* entity = nullptr;
			Ptr<DX12Buffer> vertexBuffer;
			ref<Material> computeMaterial; // holds the curve parameters of the spline
			ref<Rasterization::DX12RasterizationPipelineModule> rasterModule;
		};

		DX12GPUResourceManager* m_resourceManager = nullptr;
		std::vector<SplineGPUData> m_splines;
	};
}
