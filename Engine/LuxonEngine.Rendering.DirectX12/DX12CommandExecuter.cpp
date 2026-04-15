#include "pch.h"
#include "DX12CommandExecuter.h"

QuantumEngine::Rendering::DX12::DX12CommandExecuter::DX12CommandExecuter(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence1> fence)
	: m_commandQueue(commandQueue), m_fence(fence), 
	m_fence_value(0), m_sync_event(CreateEventW(nullptr, FALSE, FALSE, nullptr))
{
}

QuantumEngine::Rendering::DX12::DX12CommandExecuter::~DX12CommandExecuter()
{
	CloseHandle(m_sync_event);
}

void QuantumEngine::Rendering::DX12::DX12CommandExecuter::ExecuteAndWait(ID3D12GraphicsCommandList7* commandList, UInt32 timeOutMilliSeconds)
{
	commandList->Close();
	ID3D12CommandList* list[] = { commandList };
	m_commandQueue->ExecuteCommandLists(1, list);
	m_commandQueue->Signal(m_fence.Get(), ++m_fence_value);
	m_fence->SetEventOnCompletion(m_fence_value, m_sync_event);
	WaitForSingleObject(m_sync_event, timeOutMilliSeconds);
}
