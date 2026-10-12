#include "pch.h"
#include "DX12RendererSetupRegistry.h"
#include <Rendering/Renderer.h>

namespace LuxonEngine::Rendering::DX12 {
	void DX12RendererSetupRegistry::Register(RendererTypeId typeId, Ptr<IDX12RendererSetup> setup)
	{
		if (setup == nullptr)
			return;

		for (auto& entry : m_entries) {
			if (entry.typeId == typeId) {
				entry.setup = std::move(setup);
				return;
			}
		}

		m_entries.push_back(Entry{ typeId, std::move(setup) });
	}

	void DX12RendererSetupRegistry::Setup(const std::vector<DX12RendererEntity>& entities, DX12PassSetupContext& context)
	{
		std::vector<DX12RendererEntity> group;

		for (auto& entry : m_entries) {
			group.clear();

			for (auto& rendererEntity : entities) {
				if (rendererEntity.renderer != nullptr && rendererEntity.renderer->GetTypeId() == entry.typeId)
					group.push_back(rendererEntity);
			}

			if (group.empty() == false)
				entry.setup->Setup(group, context);
		}
	}

	void DX12RendererSetupRegistry::Update()
	{
		for (auto& entry : m_entries)
			entry.setup->Update();
	}

	void DX12RendererSetupRegistry::Teardown()
	{
		for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it)
			it->setup->Teardown();
	}
}
