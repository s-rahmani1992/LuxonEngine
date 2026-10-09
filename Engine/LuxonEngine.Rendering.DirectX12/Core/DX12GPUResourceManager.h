#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12RenderTexture.h"
#include "DX12DepthTexture.h"
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

		Ptr<DX12Texture> CreateTexture(const DX12TextureDesc& desc);
		Ptr<DX12RenderTexture> CreateRenderTexture(const DX12TextureDesc& desc);
		Ptr<DX12DepthTexture> CreateDepthTexture(const DX12TextureDesc& desc);

		void Destroy(Ptr<DX12Buffer>& buffer);
		void Destroy(Ptr<DX12Texture>& texture);
		void Destroy(Ptr<DX12RenderTexture>& texture);
		void Destroy(Ptr<DX12DepthTexture>& texture);

		void BeginFrame();

		void FlushPendingReleases();

	private:
		struct PendingRelease
		{
			UInt64 frameIndex;
			std::variant<Ptr<DX12Buffer>, Ptr<DX12Texture>> resource;
		};

		struct DescriptorHeap
		{
			ComPtr<ID3D12DescriptorHeap> heap;
			D3D12_CPU_DESCRIPTOR_HANDLE start = {};
		};

		struct DescriptorPool
		{
			D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			UInt32 descriptorSize = 0;
			std::vector<DescriptorHeap> heaps;
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> freeDescriptors; // handles of destroyed resources
			UInt32 currentHeapIndex = 0;   // heap the cursor is in
			UInt32 currentHeapOffset = 0;  // next unused slot in that heap
		};

		static constexpr UInt32 DescriptorsPerHeap = 1024;

		Ptr<DX12Buffer> CreateBuffer(const DX12BufferDesc& desc, UInt32 count, UInt32 stride, bool isConstant);

		bool CreateTextureResource(const DX12TextureDesc& desc, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT resourceFormat, DXGI_FORMAT srvFormat,
			const D3D12_CLEAR_VALUE* clearValue, ComPtr<ID3D12Resource2>& resource,
			D3D12_CPU_DESCRIPTOR_HANDLE& srvHandle, D3D12_CPU_DESCRIPTOR_HANDLE& uavHandle);

		bool AllocateDescriptor(DescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE& handle);
		void FreeDescriptor(DescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void RetireResource(DX12Buffer& buffer);
		void RetireResource(DX12Texture& texture);

		ID3D12Device10* m_device;
		UInt32 m_framesInFlight;
		UInt64 m_frameIndex = 0;
		std::deque<PendingRelease> m_pendingReleases;

		DescriptorPool m_cbvSrvUavPool;
		DescriptorPool m_rtvPool;
		DescriptorPool m_dsvPool;
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
