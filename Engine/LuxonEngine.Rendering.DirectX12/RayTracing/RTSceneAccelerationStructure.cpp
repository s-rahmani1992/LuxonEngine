#include "pch.h"
#include "RTSceneAccelerationStructure.h"
#include "Core/Transform.h"
#include <Core/Matrix4.h>
#include "DX12MeshController.h"
#include "DX12Utilities.h"
#include <algorithm>

namespace LuxonEngine::Rendering::DX12::RayTracing {
	RTSceneAccelerationStructure::RTSceneAccelerationStructure(ID3D12Device10* device)
		:m_device(device)
	{
	}

	ComPtr<ID3D12Resource2> RTSceneAccelerationStructure::GetOrBuildBottomLevel(ID3D12GraphicsCommandList7* commandList, const ref<DX12MeshController>& mesh)
	{
		auto it = m_bottomLevels.find(mesh.get());

		if (it != m_bottomLevels.end())
			return it->second.resource;

		ComPtr<ID3D12Resource2> scratch;
		ComPtr<ID3D12GraphicsCommandList7> commandListRef(commandList);
		ComPtr<ID3D12Resource2> resource = mesh->CreateBLASResource(commandListRef, scratch);

		if (resource == nullptr)
			return nullptr;

		m_scratchBuffers.push_back(scratch);
		m_bottomLevels.emplace(mesh.get(), BottomLevel{ mesh, resource });
		return resource;
	}

	void RTSceneAccelerationStructure::ReleaseUnusedBottomLevels(const std::unordered_set<DX12MeshController*>& usedMeshes)
	{
		for (auto it = m_bottomLevels.begin(); it != m_bottomLevels.end();) {
			if (usedMeshes.contains(it->first))
				++it;
			else
				it = m_bottomLevels.erase(it);
		}
	}

	void RTSceneAccelerationStructure::ReleaseScratchBuffers()
	{
		m_scratchBuffers.clear();
	}

	bool RTSceneAccelerationStructure::EnsureInstanceBuffer(UInt32 instanceCount)
	{
		if (instanceCount <= m_instanceCapacity && m_instanceBuffer != nullptr)
			return true;

		// grows geometrically, so adding entities one by one does not recreate the buffer every time
		UInt32 capacity = std::max({ instanceCount, m_instanceCapacity * 2, 16u });

		D3D12_RESOURCE_DESC bufferDesc = ResourceUtilities::GetCommonBufferResourceDesc(
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * capacity, D3D12_RESOURCE_FLAG_NONE);

		ComPtr<ID3D12Resource2> buffer;

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer))))
			return false;

		void* mapped = nullptr;
		D3D12_RANGE noRead{ 0, 0 };

		if (FAILED(buffer->Map(0, &noRead, &mapped)))
			return false;

		m_instanceBuffer = buffer;
		m_instanceDescs = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapped);
		m_instanceCapacity = capacity;
		return true;
	}

	void RTSceneAccelerationStructure::FillInstanceDescs()
	{
		for (UInt32 i = 0; i < m_instances.size(); i++) {
			auto& instance = m_instances[i];
			D3D12_RAYTRACING_INSTANCE_DESC& desc = m_instanceDescs[i];

			Matrix4 matrix = instance.transform != nullptr ? instance.transform->Matrix() : Matrix4();
			std::memcpy(desc.Transform, &matrix, 12 * sizeof(Float));
			desc.InstanceID = instance.instanceId;
			desc.InstanceContributionToHitGroupIndex = instance.hitGroupIndex;
			desc.InstanceMask = instance.bottomLevel != nullptr ? 1 : 0;
			desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			desc.AccelerationStructure = instance.bottomLevel != nullptr ? instance.bottomLevel->GetGPUVirtualAddress() : 0;
		}
	}

	bool RTSceneAccelerationStructure::Rebuild(ID3D12GraphicsCommandList7* commandList, const std::vector<Instance>& instances)
	{
		m_instances = instances;
		UInt32 instanceCount = (UInt32)m_instances.size();

		if (EnsureInstanceBuffer(instanceCount) == false)
			return false;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{
			.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
			.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
			.NumDescs = instanceCount,
			.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
			.InstanceDescs = m_instanceBuffer->GetGPUVirtualAddress(),
		};

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

		// the scratch buffer is shared by the build and the updates
		UInt64 scratchSize = std::max(prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);

		D3D12_RESOURCE_DESC scratchDesc = ResourceUtilities::GetCommonBufferResourceDesc(scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12_RESOURCE_DESC resultDesc = ResourceUtilities::GetCommonBufferResourceDesc(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		ComPtr<ID3D12Resource2> scratch;
		ComPtr<ID3D12Resource2> result;

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &scratchDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&scratch))))
			return false;

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resultDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&result))))
			return false;

		m_topLevelScratch = scratch;
		m_topLevel = result;

		FillInstanceDescs();
		BuildTopLevel(commandList, false);
		return true;
	}

	void RTSceneAccelerationStructure::UpdateTransforms(ID3D12GraphicsCommandList7* commandList)
	{
		if (m_topLevel == nullptr || m_instances.empty())
			return;

		FillInstanceDescs();
		BuildTopLevel(commandList, true);
	}

	void RTSceneAccelerationStructure::BuildTopLevel(ID3D12GraphicsCommandList7* commandList, bool update)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.Inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
			.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
			.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
			.NumDescs = (UInt32)m_instances.size(),
			.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
			.InstanceDescs = m_instanceBuffer->GetGPUVirtualAddress(),
		};

		buildDesc.DestAccelerationStructureData = m_topLevel->GetGPUVirtualAddress();
		buildDesc.ScratchAccelerationStructureData = m_topLevelScratch->GetGPUVirtualAddress();

		if (update) {
			buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			buildDesc.SourceAccelerationStructureData = m_topLevel->GetGPUVirtualAddress();
		}

		commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		// the rays read the structure after the build is finished
		D3D12_RESOURCE_BARRIER uavBarrier{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.UAV = D3D12_RESOURCE_UAV_BARRIER{ .pResource = m_topLevel.Get() },
		};
		commandList->ResourceBarrier(1, &uavBarrier);
	}
}
