#include "pch.h"
#include "DX12Buffer.h"

namespace LuxonEngine::Rendering::DX12 {
	DX12Buffer::~DX12Buffer()
	{
		Release();
	}

	void DX12Buffer::Release()
	{
		m_resource.Reset();
		m_cpuReadHandle = {};
		m_cpuWriteHandle = {};
		m_count = 0;
		m_stride = 0;
		m_state = D3D12_RESOURCE_STATE_COMMON;
	}

	D3D12_VERTEX_BUFFER_VIEW DX12Buffer::GetVertexBufferView() const
	{
		return D3D12_VERTEX_BUFFER_VIEW{
			.BufferLocation = m_resource->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(m_count * m_stride),
			.StrideInBytes = m_stride,
		};
	}
}
