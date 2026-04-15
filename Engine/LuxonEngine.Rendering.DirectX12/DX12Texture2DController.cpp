#include "pch.h"
#include "DX12Texture2DController.h"
#include "Core/Texture2D.h"
#include "DX12Utilities.h"

const std::map<QuantumEngine::TextureFormat, DXGI_FORMAT> QuantumEngine::Rendering::DX12::DX12Texture2DController::m_texFormatMaps{
	{QuantumEngine::TextureFormat::Unknown, DXGI_FORMAT_UNKNOWN},
	{QuantumEngine::TextureFormat::RGBA32, DXGI_FORMAT_R8G8B8A8_UNORM},
	{QuantumEngine::TextureFormat::BGRA32, DXGI_FORMAT_B8G8R8A8_UNORM},
};

QuantumEngine::Rendering::DX12::DX12Texture2DController::DX12Texture2DController(const ref<Texture2D>& texture)
	:m_texture(texture)
{
}

bool QuantumEngine::Rendering::DX12::DX12Texture2DController::Initialize(const ComPtr<ID3D12Device10>& device) {
	m_dxFormat = m_texFormatMaps.at(m_texture->GetFormat());
	
	D3D12_RESOURCE_DESC bufferDesc
	{
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	bufferDesc.Width = m_texture->GetWidth(),
	bufferDesc.Height = m_texture->GetHeight(),
	bufferDesc.DepthOrArraySize = 1,
	bufferDesc.MipLevels = 1,
	bufferDesc.Format = m_dxFormat,
	bufferDesc.SampleDesc.Count = 1,
	bufferDesc.SampleDesc.Quality = 0,
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	if (FAILED(device->CreateCommittedResource(&DX12::DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_tectureResource))))
		return false;

	D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVDesc{
	.Format = m_dxFormat,
	.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
	.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	.Texture2D = D3D12_TEX2D_SRV{.MipLevels = bufferDesc.MipLevels,},
	};

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	if(FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_textureHeap)))) {
		return false;
	}

	device->CreateShaderResourceView(m_tectureResource, &textureSRVDesc, m_textureHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void QuantumEngine::Rendering::DX12::DX12Texture2DController::UploadToGPU(ComPtr<ID3D12GraphicsCommandList7>& uploadCommandList, const ComPtr<ID3D12Resource2>& uploadTextureBuffer)
{
	auto meshData = m_texture->GetData();
	auto size = m_texture->GetTotalSize();
	void* uploadAddress;
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = size - 1;
	uploadTextureBuffer->Map(0, &range, &uploadAddress);
	std::memcpy(uploadAddress, meshData, range.End);
	uploadTextureBuffer->Unmap(0, &range);
	D3D12_BOX srcBox{
		.left = 0,
		.top = 0,
		.front = 0,
		.right = m_texture->GetWidth(),
		.bottom = m_texture->GetHeight(),
		.back = 1,
	};

	D3D12_TEXTURE_COPY_LOCATION srcTexLocation{};
	srcTexLocation.pResource = uploadTextureBuffer.Get();
	srcTexLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcTexLocation.PlacedFootprint.Offset = 0;
	srcTexLocation.PlacedFootprint.Footprint.Width = m_texture->GetWidth();
	srcTexLocation.PlacedFootprint.Footprint.Height = m_texture->GetHeight();
	srcTexLocation.PlacedFootprint.Footprint.Depth = 1;
	srcTexLocation.PlacedFootprint.Footprint.RowPitch = m_texture->GetWidth() * m_texture->GetBytePerPixel();
	srcTexLocation.PlacedFootprint.Footprint.Format = m_dxFormat;

	D3D12_TEXTURE_COPY_LOCATION destTexLocation{};
	destTexLocation.pResource = m_tectureResource;
	destTexLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destTexLocation.SubresourceIndex = 0;
	uploadCommandList->CopyTextureRegion(&destTexLocation, 0, 0, 0, &srcTexLocation, &srcBox);
}

void QuantumEngine::Rendering::DX12::DX12Texture2DController::Release() {
	m_tectureResource->Release();
	m_textureHeap->Release();
}
