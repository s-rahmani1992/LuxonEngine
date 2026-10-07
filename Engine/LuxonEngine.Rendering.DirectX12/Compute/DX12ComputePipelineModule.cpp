#include "pch.h"
#include "DX12ComputePipelineModule.h"
#include "HLSLComputeProgram.h"
#include "Rendering/Material.h"

namespace LuxonEngine::Rendering::DX12::Compute {
	DX12ComputePipelineModule::DX12ComputePipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState, ID3D12RootSignature* rootSignature, Material* material)
		:m_pipelineState(pipelineState), m_rootSignature(rootSignature)
	{
		auto* computeProgram = dynamic_cast<HLSLComputeProgram*>(material->GetProgram().get());

		m_resourceManager = std::make_unique<DX12MaterialResourceManager>(device, material, computeProgram->GetVariableReflection(),
			computeProgram->GetRootParameterLayout(), DescriptorBindPoint::Compute);
	}

	DX12ComputePipelineModule::~DX12ComputePipelineModule() = default;

	bool DX12ComputePipelineModule::Initialize(UInt32 entityCount)
	{
		if (m_resourceManager->Initialize(entityCount) == false)
			return false;

		m_threadGroupCounts.resize(entityCount);
		return true;
	}

	void DX12ComputePipelineModule::UpdateModifiedTextures()
	{
		m_resourceManager->UpdateModifiedTextures();
	}

	bool DX12ComputePipelineModule::SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceManager->SetDescriptor(name, sourceHandle);
	}

	bool DX12ComputePipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, sourceHandle);
	}

	bool DX12ComputePipelineModule::SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y, UInt32 z)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == DX12MaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_threadGroupCounts.size() <= slot)
			m_threadGroupCounts.resize(slot + 1);

		m_threadGroupCounts[slot] = { x, y, z };
		return true;
	}

	void DX12ComputePipelineModule::BindGlobal(ID3D12GraphicsCommandList7* commandList)
	{
		commandList->SetPipelineState(m_pipelineState.Get());
		commandList->SetComputeRootSignature(m_rootSignature);

		m_resourceManager->BindGlobal(commandList);
	}

	bool DX12ComputePipelineModule::DispatchSlot(ID3D12GraphicsCommandList7* commandList, UInt32 slot)
	{
		if (slot >= m_threadGroupCounts.size() || m_resourceManager->GetEntity(slot) == nullptr)
			return false;

		auto& groupCount = m_threadGroupCounts[slot];

		if (groupCount[0] == 0 || groupCount[1] == 0 || groupCount[2] == 0)
			return false;

		m_resourceManager->BindEntity(commandList, slot);
		commandList->Dispatch(groupCount[0], groupCount[1], groupCount[2]);
		return true;
	}

	bool DX12ComputePipelineModule::DispatchEntity(ID3D12GraphicsCommandList7* commandList, GameEntity* entity)
	{
		UInt32 slot = m_resourceManager->FindEntity(entity);

		if (slot == DX12MaterialResourceManager::InvalidEntityIndex)
			return false;

		return DispatchSlot(commandList, slot);
	}

	void DX12ComputePipelineModule::Dispatch(ID3D12GraphicsCommandList7* commandList)
	{
		BindGlobal(commandList);

		// every registered entity is dispatched with its own group count
		for (UInt32 slot = 0; slot < m_resourceManager->GetEntitySlotCount(); slot++)
			DispatchSlot(commandList, slot);
	}
}
