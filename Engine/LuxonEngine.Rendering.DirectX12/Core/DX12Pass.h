#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	class DX12Buffer;
	class DX12Texture;

	// A resource a pass uses and the state it is in while the pass executes. Either texture or buffer is set.
	struct DX12ResourceUsage
	{
		ID3D12Resource* resource = nullptr;
		DX12Texture* texture = nullptr;
		DX12Buffer* buffer = nullptr;
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
	};

	class DX12Pass
	{
	public:
		virtual ~DX12Pass() = default;

		void Use(DX12Texture* texture, D3D12_RESOURCE_STATES state);
		void Use(DX12Buffer* buffer, D3D12_RESOURCE_STATES state);

		void UseBackBuffer(D3D12_RESOURCE_STATES state);

		inline const std::vector<DX12ResourceUsage>& GetUsages() const { return m_usages; }
		inline bool UsesBackBuffer() const { return m_usesBackBuffer; }
		inline D3D12_RESOURCE_STATES GetBackBufferState() const { return m_backBufferState; }

		virtual void Execute(ID3D12GraphicsCommandList7* commandList) = 0;

	protected:
		void ClearUsages();

	private:
		void AddUsage(const DX12ResourceUsage& usage);

		std::vector<DX12ResourceUsage> m_usages;
		bool m_usesBackBuffer = false;
		D3D12_RESOURCE_STATES m_backBufferState = D3D12_RESOURCE_STATE_COMMON;
	};
}
