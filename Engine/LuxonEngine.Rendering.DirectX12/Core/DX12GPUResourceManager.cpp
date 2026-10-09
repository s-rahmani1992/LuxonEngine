#include "pch.h"
#include "DX12GPUResourceManager.h"
#include "DX12Utilities.h"

namespace LuxonEngine::Rendering::DX12 {
	namespace {
		// Typeless resource format and SRV format for a depth format. Returns false for unsupported formats.
		bool GetDepthFormats(DXGI_FORMAT depthFormat, DXGI_FORMAT& resourceFormat, DXGI_FORMAT& srvFormat)
		{
			switch (depthFormat) {
			case DXGI_FORMAT_D16_UNORM:
				resourceFormat = DXGI_FORMAT_R16_TYPELESS;
				srvFormat = DXGI_FORMAT_R16_UNORM;
				return true;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				resourceFormat = DXGI_FORMAT_R24G8_TYPELESS;
				srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				return true;
			case DXGI_FORMAT_D32_FLOAT:
				resourceFormat = DXGI_FORMAT_R32_TYPELESS;
				srvFormat = DXGI_FORMAT_R32_FLOAT;
				return true;
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				resourceFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
				srvFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				return true;
			default:
				return false;
			}
		}
	}

	DX12GPUResourceManager::DX12GPUResourceManager(ID3D12Device10* device, UInt32 framesInFlight)
		: m_device(device), m_framesInFlight(framesInFlight)
	{
		m_cbvSrvUavPool.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_rtvPool.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		m_dsvPool.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		for (DescriptorPool* pool : { &m_cbvSrvUavPool, &m_rtvPool, &m_dsvPool })
			pool->descriptorSize = device->GetDescriptorHandleIncrementSize(pool->type);
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

		if (!AllocateDescriptor(m_cbvSrvUavPool, readHandle))
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
				if (!AllocateDescriptor(m_cbvSrvUavPool, writeHandle)) {
					FreeDescriptor(m_cbvSrvUavPool, readHandle);
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

	bool DX12GPUResourceManager::CreateTextureResource(const DX12TextureDesc& desc, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT resourceFormat, DXGI_FORMAT srvFormat,
		const D3D12_CLEAR_VALUE* clearValue, ComPtr<ID3D12Resource2>& resource,
		D3D12_CPU_DESCRIPTOR_HANDLE& srvHandle, D3D12_CPU_DESCRIPTOR_HANDLE& uavHandle)
	{
		srvHandle = {};
		uavHandle = {};

		D3D12_RESOURCE_DESC resourceDesc{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = desc.width,
			.Height = desc.height,
			.DepthOrArraySize = 1,
			.MipLevels = desc.mipLevels,
			.Format = resourceFormat,
			.SampleDesc = { .Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = flags,
		};

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, desc.initialState, clearValue, IID_PPV_ARGS(&resource))))
			return false;

		if (!desc.name.empty())
			resource->SetName(desc.name.c_str());

		if (!(flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) {
			if (!AllocateDescriptor(m_cbvSrvUavPool, srvHandle))
				return false;

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
				.Format = srvFormat,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D = {
					.MostDetailedMip = 0,
					.MipLevels = desc.mipLevels,
					.PlaneSlice = 0,
					.ResourceMinLODClamp = 0.0f,
				},
			};
			m_device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
		}

		if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {
			if (!AllocateDescriptor(m_cbvSrvUavPool, uavHandle)) {
				FreeDescriptor(m_cbvSrvUavPool, srvHandle);
				srvHandle = {};
				return false;
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
				.Format = desc.format,
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D = {
					.MipSlice = 0,
					.PlaneSlice = 0,
				},
			};
			m_device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, uavHandle);
		}
		return true;
	}

	Ptr<DX12Texture> DX12GPUResourceManager::CreateTexture(const DX12TextureDesc& desc)
	{
		if (desc.width == 0 || desc.height == 0 || desc.format == DXGI_FORMAT_UNKNOWN)
			return nullptr;

		const bool allowsClear = (desc.flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0;
		const D3D12_CLEAR_VALUE* clearValue = (allowsClear && desc.clearValue.has_value()) ? &desc.clearValue.value() : nullptr;

		ComPtr<ID3D12Resource2> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, uavHandle;
		if (!CreateTextureResource(desc, desc.flags, desc.format, desc.format, clearValue, resource, srvHandle, uavHandle))
			return nullptr;

		auto texture = std::make_unique<DX12Texture>(resource, desc.width, desc.height, desc.mipLevels, desc.format, srvHandle, uavHandle);
		texture->SetState(desc.initialState);
		return texture;
	}

	Ptr<DX12RenderTexture> DX12GPUResourceManager::CreateRenderTexture(const DX12TextureDesc& desc)
	{
		if (desc.width == 0 || desc.height == 0 || desc.format == DXGI_FORMAT_UNKNOWN)
			return nullptr;

		const D3D12_RESOURCE_FLAGS flags = desc.flags | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		const D3D12_CLEAR_VALUE* clearValue = desc.clearValue.has_value() ? &desc.clearValue.value() : nullptr;

		ComPtr<ID3D12Resource2> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, uavHandle;
		if (!CreateTextureResource(desc, flags, desc.format, desc.format, clearValue, resource, srvHandle, uavHandle))
			return nullptr;

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
		if (!AllocateDescriptor(m_rtvPool, rtvHandle)) {
			FreeDescriptor(m_cbvSrvUavPool, srvHandle);
			FreeDescriptor(m_cbvSrvUavPool, uavHandle);
			return nullptr;
		}

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{
			.Format = desc.format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = {
				.MipSlice = 0,
				.PlaneSlice = 0,
			},
		};
		m_device->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);

		auto texture = std::make_unique<DX12RenderTexture>(resource, desc.width, desc.height, desc.mipLevels, desc.format, srvHandle, uavHandle, rtvHandle);
		texture->SetState(desc.initialState);
		return texture;
	}

	Ptr<DX12DepthTexture> DX12GPUResourceManager::CreateDepthTexture(const DX12TextureDesc& desc)
	{
		if (desc.width == 0 || desc.height == 0)
			return nullptr;

		DXGI_FORMAT resourceFormat, srvFormat;
		if (!GetDepthFormats(desc.format, resourceFormat, srvFormat))
			return nullptr;

		// A depth stencil cannot also be a render target or UAV.
		const D3D12_RESOURCE_FLAGS flags = (desc.flags | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
			& ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		// The clear value must use the depth format, never the typeless one.
		D3D12_CLEAR_VALUE clearValue{ .Format = desc.format, .DepthStencil = { .Depth = 1.0f, .Stencil = 0 } };
		if (desc.clearValue.has_value()) {
			clearValue = desc.clearValue.value();
			clearValue.Format = desc.format;
		}

		ComPtr<ID3D12Resource2> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, uavHandle;
		if (!CreateTextureResource(desc, flags, resourceFormat, srvFormat, &clearValue, resource, srvHandle, uavHandle))
			return nullptr;

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
		if (!AllocateDescriptor(m_dsvPool, dsvHandle)) {
			FreeDescriptor(m_cbvSrvUavPool, srvHandle);
			return nullptr;
		}

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
			.Format = desc.format,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE,
			.Texture2D = { .MipSlice = 0 },
		};
		m_device->CreateDepthStencilView(resource.Get(), &dsvDesc, dsvHandle);

		auto texture = std::make_unique<DX12DepthTexture>(resource, desc.width, desc.height, desc.mipLevels, desc.format, srvHandle, dsvHandle);
		texture->SetState(desc.initialState);
		return texture;
	}

	bool DX12GPUResourceManager::AllocateDescriptor(DescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE& handle)
	{
		if (!pool.freeDescriptors.empty()) {
			handle = pool.freeDescriptors.back();
			pool.freeDescriptors.pop_back();
			return true;
		}

		if (pool.heaps.empty() || pool.currentHeapOffset == DescriptorsPerHeap) {
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
				.Type = pool.type,
				.NumDescriptors = DescriptorsPerHeap,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // CPU only, modules copy from these into their shader visible heaps
			};
			DescriptorHeap newHeap;
			if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeap.heap))))
				return false;
			newHeap.start = newHeap.heap->GetCPUDescriptorHandleForHeapStart();
			pool.heaps.push_back(std::move(newHeap));
			pool.currentHeapIndex = static_cast<UInt32>(pool.heaps.size() - 1);
			pool.currentHeapOffset = 0;
		}

		handle = pool.heaps[pool.currentHeapIndex].start;
		handle.ptr += static_cast<SIZE_T>(pool.currentHeapOffset) * pool.descriptorSize;
		++pool.currentHeapOffset;
		return true;
	}

	void DX12GPUResourceManager::FreeDescriptor(DescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		if (handle.ptr != 0)
			pool.freeDescriptors.push_back(handle);
	}

	void DX12GPUResourceManager::RetireResource(DX12Buffer& buffer)
	{
		FreeDescriptor(m_cbvSrvUavPool, buffer.GetReadHandle());
		FreeDescriptor(m_cbvSrvUavPool, buffer.GetWriteHandle());
		buffer.Release();
	}

	void DX12GPUResourceManager::RetireResource(DX12Texture& texture)
	{
		FreeDescriptor(m_cbvSrvUavPool, texture.GetSrvHandle());
		FreeDescriptor(m_cbvSrvUavPool, texture.GetUavHandle());
		if (auto* renderTexture = dynamic_cast<DX12RenderTexture*>(&texture))
			FreeDescriptor(m_rtvPool, renderTexture->GetRtvHandle());
		else if (auto* depthTexture = dynamic_cast<DX12DepthTexture*>(&texture))
			FreeDescriptor(m_dsvPool, depthTexture->GetDsvHandle());
		texture.Release();
	}

	void DX12GPUResourceManager::Destroy(Ptr<DX12Buffer>& buffer)
	{
		if (buffer == nullptr)
			return;
		m_pendingReleases.push_back({ m_frameIndex, std::move(buffer) });
		buffer = nullptr;
	}

	void DX12GPUResourceManager::Destroy(Ptr<DX12Texture>& texture)
	{
		if (texture == nullptr)
			return;
		m_pendingReleases.push_back({ m_frameIndex, std::move(texture) });
		texture = nullptr;
	}

	void DX12GPUResourceManager::Destroy(Ptr<DX12RenderTexture>& texture)
	{
		Ptr<DX12Texture> base = std::move(texture);
		texture = nullptr;
		Destroy(base);
	}

	void DX12GPUResourceManager::Destroy(Ptr<DX12DepthTexture>& texture)
	{
		Ptr<DX12Texture> base = std::move(texture);
		texture = nullptr;
		Destroy(base);
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
