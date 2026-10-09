#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	class DX12Buffer;
	class DX12Texture;

	class DX12Pass
	{
	public:
		virtual ~DX12Pass() = default;

		void AddTransition(DX12Texture* texture, D3D12_RESOURCE_STATES requiredState);
		void AddTransition(DX12Buffer* buffer, D3D12_RESOURCE_STATES requiredState);

		void Execute(ID3D12GraphicsCommandList7* commandList);

	protected:
		virtual void Record(ID3D12GraphicsCommandList7* commandList) = 0;

		inline void ClearTransitions() { m_transitions.clear(); }

	private:
		struct Transition
		{
			DX12Texture* texture = nullptr;
			DX12Buffer* buffer = nullptr;
			D3D12_RESOURCE_STATES requiredState = D3D12_RESOURCE_STATE_COMMON;
		};

		void ApplyTransitions(ID3D12GraphicsCommandList7* commandList);

		std::vector<Transition> m_transitions;
	};
}
