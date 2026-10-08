#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <string>

namespace LuxonEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	enum class DX12MemoryType
	{
		GPUOnly,
		Upload,
	};

	struct DX12BufferDesc
	{
		DX12MemoryType memoryType = DX12MemoryType::GPUOnly;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;		
		std::wstring name;
	};

	class DX12Buffer
	{
	public:
		// The read handle is a CBV or SRV and the write handle a UAV (zero when the buffer has none).
		// The handles point into heaps owned by DX12GPUResourceManager.
		DX12Buffer(ComPtr<ID3D12Resource2> resource, UInt32 count, UInt32 stride, D3D12_CPU_DESCRIPTOR_HANDLE cpuReadHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuWriteHandle) 
			: m_resource(resource), m_count(count), m_stride(stride), m_cpuReadHandle(cpuReadHandle), m_cpuWriteHandle(cpuWriteHandle) {}
		~DX12Buffer();

		DX12Buffer(const DX12Buffer&) = delete;
		DX12Buffer& operator=(const DX12Buffer&) = delete;

		void Release();

		inline ID3D12Resource2* GetResource() const { return m_resource.Get(); }
		inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_resource->GetGPUVirtualAddress(); }
		
		inline UInt32 GetCount() const { return m_count; }
		inline UInt32 GetStride() const { return m_stride; }
		inline UInt64 GetSize() const { return static_cast<UInt64>(m_count) * m_stride; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE GetReadHandle() const { return m_cpuReadHandle; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE GetWriteHandle() const { return m_cpuWriteHandle; }
		inline bool HasReadHandle() const { return m_cpuReadHandle.ptr != 0; }
		inline bool HasWriteHandle() const { return m_cpuWriteHandle.ptr != 0; }

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
		
		inline D3D12_RESOURCE_STATES GetState() const { return m_state; }
		inline void SetState(D3D12_RESOURCE_STATES state) { m_state = state; }

	private:
		ComPtr<ID3D12Resource2> m_resource;
		UInt32 m_count = 0;
		UInt32 m_stride = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuReadHandle = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuWriteHandle = {};
		D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_COMMON;
	};
}
