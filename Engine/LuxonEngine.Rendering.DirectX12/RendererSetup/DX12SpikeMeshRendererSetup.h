#pragma once
#include "Core/DX12RendererSetup.h"

namespace LuxonEngine::Rendering::DX12 {
	// Draws the spike mesh renderers: one mesh shading pipeline per material, added to the main render pass.
	class DX12SpikeMeshRendererSetup : public IDX12RendererSetup
	{
	public:
		virtual void Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context) override;
	};
}
