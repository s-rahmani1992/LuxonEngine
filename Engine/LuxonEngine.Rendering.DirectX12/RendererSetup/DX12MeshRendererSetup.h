#pragma once
#include "Core/DX12RendererSetup.h"

namespace LuxonEngine::Rendering::DX12 {
	// Draws the mesh renderers: one rasterization pipeline per material, added to the main render pass.
	class DX12MeshRendererSetup : public IDX12RendererSetup
	{
	public:
		virtual void Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context) override;
	};
}
