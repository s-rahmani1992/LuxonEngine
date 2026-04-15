#include "pch.h"
#include "DX12LightManager.h"
#include <BasicTypes.h>
#include "DX12Utilities.h"

bool QuantumEngine::Rendering::DX12::DX12LightManager::Initialize(const SceneLightData& lights, const ComPtr<ID3D12Device10>& device)
{
	UInt32 lightSize = CONSTANT_BUFFER_ALIGHT(10 * (sizeof(DirectionalLight) + sizeof(PointLight)) + 2 * sizeof(UInt32));

	D3D12_RESOURCE_DESC shaderTableBufferDesc
	{
	shaderTableBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	shaderTableBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	shaderTableBufferDesc.Width = lightSize,
	shaderTableBufferDesc.Height = 1,
	shaderTableBufferDesc.DepthOrArraySize = 1,
	shaderTableBufferDesc.MipLevels = 1,
	shaderTableBufferDesc.Format = DXGI_FORMAT_UNKNOWN,
	shaderTableBufferDesc.SampleDesc.Count = 1,
	shaderTableBufferDesc.SampleDesc.Quality = 0,
	shaderTableBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	shaderTableBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
		&shaderTableBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_lightBuffer))))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_lightHeap)))) {
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc1{
		.BufferLocation = m_lightBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = lightSize,
	};

	device->CreateConstantBufferView(&cbDesc1, m_lightHeap->GetCPUDescriptorHandleForHeapStart());
	
	Byte* pData;
	m_lightBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
	std::memcpy(pData, (void*)lights.directionalLights.data(), lights.directionalLights.size() * sizeof(DirectionalLight));
	pData += 10 * (sizeof(DirectionalLight));
	std::memcpy(pData, (void*)lights.pointLights.data(), lights.pointLights.size() * sizeof(PointLight));
	pData += 10 * (sizeof(PointLight));
	UInt32* data = reinterpret_cast<UInt32*>(pData);
	*data = lights.directionalLights.size();
	data++;
	*data = lights.pointLights.size();
	m_lightBuffer->Unmap(0, nullptr);
	return true;
}
