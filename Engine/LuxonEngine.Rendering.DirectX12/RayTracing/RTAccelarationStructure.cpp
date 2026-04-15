#include "../pch.h"
#include "RTAccelarationStructure.h"
#include "Core/Transform.h"
#include <Core/Matrix4.h>
#include "../DX12MeshController.h"
#include "../DX12Utilities.h"

bool QuantumEngine::Rendering::DX12::RayTracing::RTAccelarationStructure::Initialize(const ComPtr<ID3D12GraphicsCommandList7>& commandList, const std::vector<EntityBLASDesc>& entities)
{
	ComPtr<ID3D12Device10> device;

	if (FAILED(commandList->GetDevice(IID_PPV_ARGS(&device))))
		return false;

	scratchBuffers = std::vector<ComPtr<ID3D12Resource2>>(entities.size());
	UInt32 index = 0;

	for(auto& entity : entities) {
		auto r = entity.mesh->CreateBLASResource(commandList, scratchBuffers[index]);
		m_entities.push_back({ entity.transform, entity.hitGroupIndex, r });
		index++;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	topLevelInputs.NumDescs = entities.size();
	topLevelInputs.pGeometryDescs = nullptr;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

	D3D12_RESOURCE_DESC rtScratchBufDesc = {};
	rtScratchBufDesc.Alignment = 0;
	rtScratchBufDesc.DepthOrArraySize = 1;
	rtScratchBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rtScratchBufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	rtScratchBufDesc.Format = DXGI_FORMAT_UNKNOWN;
	rtScratchBufDesc.Height = 1;
	rtScratchBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rtScratchBufDesc.MipLevels = 1;
	rtScratchBufDesc.SampleDesc.Count = 1;
	rtScratchBufDesc.SampleDesc.Quality = 0;
	rtScratchBufDesc.Width = topLevelPrebuildInfo.ScratchDataSizeInBytes;

	D3D12_RESOURCE_DESC rtResultBufDesc = rtScratchBufDesc;
	rtResultBufDesc.Width = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &rtScratchBufDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_rtScratchBuffer))))
		return false;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &rtResultBufDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&m_topLevelAccelerationStructure))))
		return false;

	UInt32 id = 0;
	Matrix4 m;

	for (auto& entity : m_entities) {
		D3D12_RAYTRACING_INSTANCE_DESC desc1;
		m = entity.transform->Matrix();
		std::memcpy(desc1.Transform, &m, 12 * sizeof(Float));
		desc1.InstanceID = entity.hitIndex;
		desc1.InstanceContributionToHitGroupIndex = entity.hitIndex;
		desc1.InstanceMask = 1;
		desc1.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		desc1.AccelerationStructure = entity.BLASResource->GetGPUVirtualAddress();

		m_instanceDescs.push_back(desc1);
		id++;
	}

	D3D12_RESOURCE_DESC tlasBufferDesc
	{
	tlasBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	tlasBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	tlasBufferDesc.Width = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * m_instanceDescs.size(),
	tlasBufferDesc.Height = 1,
	tlasBufferDesc.DepthOrArraySize = 1,
	tlasBufferDesc.MipLevels = 1,
	tlasBufferDesc.Format = DXGI_FORMAT_UNKNOWN,
	tlasBufferDesc.SampleDesc.Count = 1,
	tlasBufferDesc.SampleDesc.Quality = 0,
	tlasBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	tlasBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE, &tlasBufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_tlasUpload))))
		return false;

	auto meshData = m_instanceDescs.data();
	auto size = tlasBufferDesc.Width;
	void* uploadAddress;
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = size - 1;
	m_tlasUpload->Map(0, &range, &uploadAddress);
	std::memcpy(uploadAddress, meshData, range.End);
	m_tlasUpload->Unmap(0, &range);

	topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_rtScratchBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.Inputs.InstanceDescs = m_tlasUpload->GetGPUVirtualAddress();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_tlasHeap)))) {
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC tlasSRVDesc = {};
	tlasSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	tlasSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tlasSRVDesc.RaytracingAccelerationStructure.Location = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
	device->CreateShaderResourceView(nullptr, &tlasSRVDesc, m_tlasHeap->GetCPUDescriptorHandleForHeapStart());
	commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
}

void QuantumEngine::Rendering::DX12::RayTracing::RTAccelarationStructure::UpdateTransforms(const ComPtr<ID3D12GraphicsCommandList7>& commandList, Matrix4& viewMatrix)
{
	Matrix4 m;

	for (int i = 0; i < m_entities.size(); i++) {
		m = (m_entities[i].transform->Matrix());
		std::memcpy(&m_instanceDescs[i].Transform, &m, 12 * sizeof(Float));
	}

	void* uploadAddress;
	m_tlasUpload->Map(0, nullptr, &uploadAddress);
	std::memcpy(uploadAddress, m_instanceDescs.data(), m_instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	m_tlasUpload->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	topLevelInputs.NumDescs = m_instanceDescs.size();
	topLevelInputs.pGeometryDescs = nullptr;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	topLevelBuildDesc.SourceAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
	topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_rtScratchBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.Inputs.InstanceDescs = m_tlasUpload->GetGPUVirtualAddress();

	commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
}
