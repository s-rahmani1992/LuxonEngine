#include "pch.h"
#include "DX12MeshPipelineModule.h"
#include "HLSLMeshProgram.h"
#include "Rendering/Material.h"

namespace LuxonEngine::Rendering::DX12::MeshShading {
	DX12MeshPipelineModule::DX12MeshPipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState, ID3D12RootSignature* rootSignature, Material* material)
		:m_pipelineState(pipelineState), m_rootSignature(rootSignature)
	{
		auto* meshProgram = dynamic_cast<HLSLMeshProgram*>(material->GetProgram().get());

		m_resourceManager = std::make_unique<DX12MaterialResourceManager>(device, material, meshProgram->GetVariableReflection(),
			meshProgram->GetRootParameterLayout(), DescriptorBindPoint::Graphics);
	}

	DX12MeshPipelineModule::~DX12MeshPipelineModule() = default;

	bool DX12MeshPipelineModule::Initialize(UInt32 entityCount)
	{
		if (m_resourceManager->Initialize(entityCount) == false)
			return false;

		m_threadGroupCounts.resize(entityCount);
		return true;
	}

	void DX12MeshPipelineModule::UpdateModifiedTextures()
	{
		m_resourceManager->UpdateModifiedTextures();
	}

	bool DX12MeshPipelineModule::SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceManager->SetDescriptor(name, sourceHandle);
	}

	bool DX12MeshPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, sourceHandle);
	}

	void DX12MeshPipelineModule::Dispatch(ID3D12GraphicsCommandList7* commandList)
	{
		commandList->SetPipelineState(m_pipelineState.Get());
		commandList->SetGraphicsRootSignature(m_rootSignature);

		m_resourceManager->BindGlobal(commandList);

		// every registered entity is dispatched with its own group count
		for (UInt32 slot = 0; slot < m_resourceManager->GetEntitySlotCount() && slot < m_threadGroupCounts.size(); slot++) {
			auto& groupCount = m_threadGroupCounts[slot];

			if (m_resourceManager->GetEntity(slot) == nullptr || groupCount[0] == 0 || groupCount[1] == 0 || groupCount[2] == 0)
				continue;

			m_resourceManager->BindEntity(commandList, slot);
			commandList->DispatchMesh(groupCount[0], groupCount[1], groupCount[2]);
		}
	}

	bool DX12MeshPipelineModule::SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y, UInt32 z)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == DX12MaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_threadGroupCounts.size() <= slot)
			m_threadGroupCounts.resize(slot + 1);

		m_threadGroupCounts[slot] = { x, y, z };
		return true;
	}
}
