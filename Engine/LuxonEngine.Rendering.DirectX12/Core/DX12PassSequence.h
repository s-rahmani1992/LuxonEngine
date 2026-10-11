#pragma once
#include "DX12Pass.h"
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	class DX12Buffer;
	class DX12Texture;

	class DX12PassSequence
	{
	public:
		void AddPass(DX12Pass* pass);
		void SetSwapChain(IDXGISwapChain3* swapChain);
		void Clear();
		bool Compile();
		void Execute(ID3D12GraphicsCommandList7* commandList);

		inline void SetLogPlan(bool logPlan) { m_logPlan = logPlan; }
		inline UInt32 GetPassCount() const { return static_cast<UInt32>(m_passes.size()); }

	private:
		struct EntryState
		{
			DX12Texture* texture = nullptr;
			DX12Buffer* buffer = nullptr;
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		};

		std::vector<DX12Pass*> m_passes;
		IDXGISwapChain3* m_swapChain = nullptr;
		bool m_logPlan = false;

		std::vector<std::vector<std::vector<D3D12_RESOURCE_BARRIER>>> m_passBarriers;
		std::vector<std::vector<D3D12_RESOURCE_BARRIER>> m_beginBarriers;
		std::vector<std::vector<D3D12_RESOURCE_BARRIER>> m_endBarriers;
		std::vector<D3D12_RESOURCE_BARRIER> m_prologueBarriers;
		std::vector<EntryState> m_entryStates;

		bool m_compiled = false;
		bool m_prologueDone = false;
	};
}
