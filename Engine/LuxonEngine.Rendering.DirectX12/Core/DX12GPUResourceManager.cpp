#include "pch.h"
#include "DX12GPUResourceManager.h"
#include "DX12Utilities.h"

namespace LuxonEngine::Rendering::DX12 {
	DX12GPUResourceManager::DX12GPUResourceManager(ID3D12Device10* device, UInt32 framesInFlight)
		: m_device(device), m_framesInFlight(framesInFlight),
		m_descriptorSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
	{
	}

	DX12GPUResourceManager::~DX12GPUResourceManager()
	{
		// The owning context is expected to have waited for the GPU before destroying the manager.
		FlushPendingReleases();
	}

	Ptr<DX12Buffer> DX12GPUResourceManager::CreateBuffer(const DX12BufferDesc& desc, UInt32 count, UInt32 stride, bool isConstant)
	{
		const UInt64 size = static_cast<UInt64>(count) * stride;
		const D3D12_HEAP_PROPERTIES* heapProps = &DescriptorUtilities::CommonDefaultHeapProps;
		D3D12_RESOURCE_STATES initialState = desc.initialState;
		D3D12_RESOURCE_FLAGS flags = desc.flags;

		if (desc.memoryType == DX12MemoryType::Upload) {
			heapProps = &DescriptorUtilities::CommonUploadHeapProps;
			initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
			flags &= ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // not allowed on upload heaps
		}
		if (isConstant)
			flags &= ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		ComPtr<ID3D12Resource2> resource;
		D3D12_RESOURCE_DESC resourceDesc = ResourceUtilities::GetCommonBufferResourceDesc(size, flags);
		if (FAILED(m_device->CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			initialState, nullptr, IID_PPV_ARGS(&resource))))
			return nullptr;

		if (!desc.name.empty())
			resource->SetName(desc.name.c_str());

		D3D12_CPU_DESCRIPTOR_HANDLE readHandle = {};
		D3D12_CPU_DESCRIPTOR_HANDLE writeHandle = {};

		if (!AllocateDescriptor(readHandle))
			return nullptr;

		if (isConstant) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
				.BufferLocation = resource->GetGPUVirtualAddress(),
				.SizeInBytes = static_cast<UInt32>(size),
			};
			m_device->CreateConstantBufferView(&cbvDesc, readHandle);
		}
		else {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer = {
					.FirstElement = 0,
					.NumElements = count,
					.StructureByteStride = stride,
					.Flags = D3D12_BUFFER_SRV_FLAG_NONE,
				},
			};
			m_device->CreateShaderResourceView(resource.Get(), &srvDesc, readHandle);

			if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {
				if (!AllocateDescriptor(writeHandle)) {
					FreeDescriptor(readHandle);
					return nullptr;
				}
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
					.Format = DXGI_FORMAT_UNKNOWN,
					.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
					.Buffer = {
						.FirstElement = 0,
						.NumElements = count,
						.StructureByteStride = stride,
						.CounterOffsetInBytes = 0,
						.Flags = D3D12_BUFFER_UAV_FLAG_NONE,
					},
				};
				m_device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, writeHandle);
			}
		}

		auto buffer = std::make_unique<DX12Buffer>(resource, count, stride, readHandle, writeHandle);
		buffer->SetState(initialState);
		return buffer;
	}

	bool DX12GPUResourceManager::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle)
	{
		if (!m_freeDescriptors.empty()) {
			handle = m_freeDescriptors.back();
			m_freeDescriptors.pop_back();
			return true;
		}

		if (m_descriptorHeaps.empty() || m_currentHeapOffset == DescriptorsPerHeap) {
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = DescriptorsPerHeap,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // CPU only, modules copy from these into their shader visible heaps
			};
			DescriptorHeap newHeap;
			if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeap.heap))))
				return false;
			newHeap.start = newHeap.heap->GetCPUDescriptorHandleForHeapStart();
			m_descriptorHeaps.push_back(std::move(newHeap));
			m_currentHeapIndex = static_cast<UInt32>(m_descriptorHeaps.size() - 1);
			m_currentHeapOffset = 0;
		}

		handle = m_descriptorHeaps[m_currentHeapIndex].start;
		handle.ptr += static_cast<SIZE_T>(m_currentHeapOffset) * m_descriptorSize;
		++m_currentHeapOffset;
		return true;
	}

	void DX12GPUResourceManager::FreeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		if (handle.ptr != 0)
			m_freeDescriptors.push_back(handle);
	}

	void DX12GPUResourceManager::RetireResource(DX12Buffer& buffer)
	{
		FreeDescriptor(buffer.GetReadHandle());
		FreeDescriptor(buffer.GetWriteHandle());
		buffer.Release();
	}

	void DX12GPUResourceManager::Destroy(Ptr<DX12Buffer>& buffer)
	{
		if (buffer == nullptr)
			return;
		m_pendingReleases.push_back({ m_frameIndex, std::move(buffer) });
		buffer = nullptr;
	}

	void DX12GPUResourceManager::BeginFrame()
	{
		++m_frameIndex;
		// Entries are queued in frame order, so only the front needs checking.
		while (!m_pendingReleases.empty() && m_pendingReleases.front().frameIndex + m_framesInFlight < m_frameIndex) {
			std::visit([this](auto& resource) { RetireResource(*resource); }, m_pendingReleases.front().resource);
			m_pendingReleases.pop_front();
		}
	}

	void DX12GPUResourceManager::FlushPendingReleases()
	{
		for (auto& pending : m_pendingReleases)
			std::visit([this](auto& resource) { RetireResource(*resource); }, pending.resource);
		m_pendingReleases.clear();
	}
}
