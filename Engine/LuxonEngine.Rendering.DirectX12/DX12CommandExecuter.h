#pragma once
#include "pch.h"
#include "BasicTypes.h"

namespace QuantumEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	class DX12CommandExecuter
	{
	public:
		DX12CommandExecuter(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence1> fence);
		~DX12CommandExecuter();
		void ExecuteAndWait(ID3D12GraphicsCommandList7* commandList, UInt32 timeOutMilliSeconds = 20000);
		inline ComPtr<ID3D12CommandQueue> GetQueue() const { return m_commandQueue; }
	private:
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<ID3D12Fence1> m_fence;
		UInt64 m_fence_value;
		HANDLE m_sync_event;
	};
}
