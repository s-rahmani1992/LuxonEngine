#include "pch.h"
#include "DX12Utilities.h"

namespace DirectX12 = QuantumEngine::Rendering::DX12;

const D3D12_HEAP_PROPERTIES DirectX12::DescriptorUtilities::CommonUploadHeapProps = D3D12_HEAP_PROPERTIES{
	.Type = D3D12_HEAP_TYPE_UPLOAD,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0, 
};

const D3D12_HEAP_PROPERTIES DirectX12::DescriptorUtilities::CommonDefaultHeapProps = D3D12_HEAP_PROPERTIES{
	.Type = D3D12_HEAP_TYPE_DEFAULT,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0,
};

D3D12_RESOURCE_DESC QuantumEngine::Rendering::DX12::ResourceUtilities::GetCommonBufferResourceDesc(UInt32 size, D3D12_RESOURCE_FLAGS flag)
{
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = flag;
	return resourceDesc;
}
