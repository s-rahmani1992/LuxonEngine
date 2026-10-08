#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12Buffer.h"
#include <deque>
#include <variant>
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	class DX12GPUResourceManager
	{
	public:
		DX12GPUResourceManager(ID3D12Device10* device, UInt32 framesInFlight);
		~DX12GPUResourceManager();

		DX12GPUResourceManager(const DX12GPUResourceManager&) = delete;
		DX12GPUResourceManager& operator=(const DX12GPUResourceManager&) = delete;

		// Read handle is an SRV. Gets a UAV write handle when desc.flags has ALLOW_UNORDERED_ACCESS. Returns nullptr on failure.
		template<typename T>
		Ptr<DX12Buffer> CreateStructuredBuffer(const DX12BufferDesc& desc, int count);

		// Read handle is a CBV. Size is rounded up to the constant buffer alignment and the UAV flag is ignored.
		template<typename T>
		Ptr<DX12Buffer> CreateConstantBuffer(const DX12BufferDesc& desc);

		// Resets the caller's pointer. The buffer and its descriptors are released once the frames in flight are done.
		void Destroy(Ptr<DX12Buffer>& buffer);

		void BeginFrame();

		void FlushPendingReleases();

	private:
		struct PendingRelease
		{
			UInt64 frameIndex;
			std::variant<Ptr<DX12Buffer>> resource;
		};

		// One non-shader-visible CBV/SRV/UAV heap. Heaps are never resized, so handed out handles stay valid.
		struct DescriptorHeap
		{
			ComPtr<ID3D12DescriptorHeap> heap;
			D3D12_CPU_DESCRIPTOR_HANDLE start = {};
		};

		static constexpr UInt32 DescriptorsPerHeap = 1024;

		Ptr<DX12Buffer> CreateBuffer(const DX12BufferDesc& desc, UInt32 count, UInt32 stride, bool isConstant);

		// Takes a handle from the freed list first, then from the cursor, adding a heap when the last one is full.
		bool AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle);
		void FreeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void RetireResource(DX12Buffer& buffer);

		ID3D12Device10* m_device;
		UInt32 m_framesInFlight;
		UInt64 m_frameIndex = 0;
		std::deque<PendingRelease> m_pendingReleases;

		UInt32 m_descriptorSize;
		std::vector<DescriptorHeap> m_descriptorHeaps;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_freeDescriptors; // handles of destroyed buffers
		UInt32 m_currentHeapIndex = 0;   // heap the cursor is in
		UInt32 m_currentHeapOffset = 0;  // next unused slot in that heap
	};

	template<typename T>
	inline Ptr<DX12Buffer> DX12GPUResourceManager::CreateStructuredBuffer(const DX12BufferDesc& desc, int count)
	{
		return CreateBuffer(desc, count, sizeof(T), false);
	}

	template<typename T>
	inline Ptr<DX12Buffer> DX12GPUResourceManager::CreateConstantBuffer(const DX12BufferDesc& desc)
	{
		constexpr UInt32 alignedSize = (sizeof(T) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		return CreateBuffer(desc, 1, alignedSize, true);
	}
}
