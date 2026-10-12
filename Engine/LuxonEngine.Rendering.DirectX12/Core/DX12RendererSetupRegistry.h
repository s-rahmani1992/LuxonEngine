#pragma once
#include "DX12RendererSetup.h"
#include <Rendering/RendererType.h>
#include <utility>
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	class DX12RendererSetupRegistry {
	public:
		template<typename RendererType, typename SetupType, typename... Args>
		void Register(Args&&... args)
		{
			Register(RendererType::StaticTypeId(), std::make_unique<SetupType>(std::forward<Args>(args)...));
		}

		void Register(RendererTypeId typeId, Ptr<IDX12RendererSetup> setup);
		void Setup(const std::vector<DX12RendererEntity>& entities, DX12PassSetupContext& context);
		void Update();
		void Teardown();

	private:
		struct Entry {
			RendererTypeId typeId = 0;
			Ptr<IDX12RendererSetup> setup;
		};

		std::vector<Entry> m_entries;
	};
}
