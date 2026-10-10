#include "pch.h"
#include "DX12RayTracingPipelineModule.h"
#include "HLSLRayTracingProgram.h"
#include "RTStateObjectBuilder.h"
#include "Rendering/Material.h"
#include "Rendering/RayTracingComponent.h"
#include "Core/GameEntity.h"
#include "Core/Logger.h"
#include "Core/Mesh.h"
#include "Core/Transform.h"
#include "RTSceneAccelerationStructure.h"
#include "DX12MeshController.h"
#include "DX12CommandExecuter.h"
#include <algorithm>

namespace LuxonEngine::Rendering::DX12::RayTracing {
	DX12RayTracingPipelineModule::DX12RayTracingPipelineModule(ID3D12Device10* device, const RayTracingPipelineProperties& properties)
		:m_device(device), m_properties(properties)
	{
	}

	bool DX12RayTracingPipelineModule::Initialize(Material* globalMaterial, std::vector<RayTracingEntityDesc> initialEntities)
	{
		if (globalMaterial == nullptr)
			return false;

		m_globalMaterial = globalMaterial;
		m_globalProgram = std::dynamic_pointer_cast<HLSLRayTracingProgram>(globalMaterial->GetProgram()).get();
		m_initialEntities = std::move(initialEntities);

		if (m_globalProgram == nullptr || m_globalProgram->GetRayGenExportName().empty())
			return false;

		// the state object depends on the programs, not on the materials or the entities
		for (auto& desc : m_initialEntities) {
			if (desc.component == nullptr || desc.component->GetRTMaterial() == nullptr)
				continue;

			auto program = std::dynamic_pointer_cast<HLSLRayTracingProgram>(desc.component->GetRTMaterial()->GetProgram());

			if (program == nullptr) {
				Logger::LogWarning("The material of " + desc.entity->GetName() + " is not a ray tracing material");
				continue;
			}

			if (std::find(m_localPrograms.begin(), m_localPrograms.end(), program.get()) == m_localPrograms.end())
				m_localPrograms.push_back(program.get());
		}

		std::string error;
		m_stateObject = RTStateObjectBuilder::Create(m_device.Get(), m_properties, m_globalProgram, m_localPrograms, error);

		if (m_stateObject == nullptr) {
			Logger::LogError(error);
			return false;
		}

		// the heap holds the descriptors of the global program and the materials of the entities
		m_resourceTable = std::make_unique<DX12RayTracingResourceTable>(m_device.Get());

		std::vector<DX12RayTracingResourceTable::EntityMaterial> entityMaterials;

		for (auto& desc : m_initialEntities) {
			if (desc.component != nullptr && desc.component->GetRTMaterial() != nullptr)
				entityMaterials.push_back({ desc.entity, desc.component->GetRTMaterial().get() });
		}

		if (m_resourceTable->Initialize(m_globalMaterial, entityMaterials) == false) {
			Logger::LogError("Failed to create the descriptor heap of the ray tracing pipeline");
			return false;
		}

		if (m_resourceTable->BuildShaderTable(m_stateObject.Get()) == false) {
			Logger::LogError("Failed to create the shader binding table of the ray tracing pipeline");
			return false;
		}

		return BuildInitialStructures();
	}

	bool DX12RayTracingPipelineModule::BuildInitialStructures()
	{
		// own queue and commands, that are only used by the initialization. it is finished before Initialize returns
		D3D12_COMMAND_QUEUE_DESC queueDesc{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0,
		};

		ComPtr<ID3D12CommandQueue> queue;
		ComPtr<ID3D12Fence1> fence;
		ComPtr<ID3D12CommandAllocator> allocator;
		ComPtr<ID3D12GraphicsCommandList7> commandList;

		if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)))
			|| FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))
			|| FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)))
			|| FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)))) {
			Logger::LogError("Failed to create the initialization commands of the ray tracing pipeline");
			return false;
		}

		DX12CommandExecuter commandExecuter(queue, fence);
		m_accelerationStructure = std::make_unique<RTSceneAccelerationStructure>(m_device.Get());

		// one instance for every entity of the resource table, in the same order. the order is the hit group index of the instance
		std::vector<RTSceneAccelerationStructure::Instance> instances;

		for (auto& desc : m_initialEntities) {
			if (desc.component == nullptr || desc.component->GetRTMaterial() == nullptr)
				continue;

			auto& instance = instances.emplace_back();
			instance.instanceId = (UInt32)instances.size() - 1;
			instance.hitGroupIndex = m_resourceTable->GetFirstEntityHitRecord() + instance.instanceId;

			auto meshController = desc.component->GetMesh() != nullptr ? std::dynamic_pointer_cast<DX12MeshController>(desc.component->GetMesh()->GetGPUHandle()) : nullptr;

			// an entity without a mesh on the GPU keeps its record, but its instance is inactive
			if (meshController == nullptr) {
				Logger::LogWarning("The mesh of " + desc.entity->GetName() + " is not uploaded to the GPU, it is not traced");
				continue;
			}

			instance.transform = desc.entity->GetTransform();
			instance.bottomLevel = m_accelerationStructure->GetOrBuildBottomLevel(commandList.Get(), meshController);
		}

		if (m_accelerationStructure->Rebuild(commandList.Get(), instances) == false) {
			Logger::LogError("Failed to build the top level acceleration structure");
			return false;
		}

		commandExecuter.ExecuteAndWait(commandList.Get());
		m_accelerationStructure->ReleaseScratchBuffers();

		m_resourceTable->SetAccelerationStructure(m_accelerationStructure->GetGPUAddress());

		m_dispatchDesc.Depth = 1;
		m_resourceTable->FillDispatchRaysDesc(m_dispatchDesc);
		return true;
	}

	bool DX12RayTracingPipelineModule::SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceTable != nullptr && m_resourceTable->SetDescriptor(name, sourceHandle);
	}

	bool DX12RayTracingPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceTable != nullptr && m_resourceTable->SetEntityDescriptor(entity, name, sourceHandle);
	}

	void DX12RayTracingPipelineModule::SetDimensions(UInt32 width, UInt32 height)
	{
		m_dispatchDesc.Width = width;
		m_dispatchDesc.Height = height;
	}

	bool DX12RayTracingPipelineModule::Dispatch(ID3D12GraphicsCommandList7* commandList)
	{
		if (m_stateObject == nullptr || m_dispatchDesc.Width == 0 || m_dispatchDesc.Height == 0)
			return false;

		m_resourceTable->UpdateModifiedTextures();
		m_resourceTable->UpdateModifiedValues();
		m_accelerationStructure->UpdateTransforms(commandList);

		commandList->SetComputeRootSignature(m_globalProgram->GetRootSignature().Get());
		commandList->SetPipelineState1(m_stateObject.Get());
		m_resourceTable->BindGlobal(commandList);
		commandList->DispatchRays(&m_dispatchDesc);
		return true;
	}
}
