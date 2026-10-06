#include "pch.h"
#include "DX12RasterizationPipelineModule.h"
#include "HLSLRasterizationProgram.h"
#include "Rendering/Material.h"

namespace LuxonEngine::Rendering::DX12::Rasterization {
	DX12RasterizationPipelineModule::DX12RasterizationPipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState, ID3D12RootSignature* rootSignature, Material* material, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
		:m_pipelineState(pipelineState), m_rootSignature(rootSignature)
	{
		switch (topologyType) {
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
			m_topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
			m_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		default:
			m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		}

		auto* rasterizationProgram = dynamic_cast<HLSLRasterizationProgram*>(material->GetProgram().get());

		m_resourceManager = std::make_unique<DX12MaterialResourceManager>(device, material, rasterizationProgram->GetVariableReflection(),
			rasterizationProgram->GetRootParameterLayout(), DescriptorBindPoint::Graphics);
	}

	DX12RasterizationPipelineModule::~DX12RasterizationPipelineModule() = default;

	bool DX12RasterizationPipelineModule::Initialize(UInt32 entityCount)
	{
		if (m_resourceManager->Initialize(entityCount) == false)
			return false;

		m_entityGeometries.resize(entityCount);
		return true;
	}

	void DX12RasterizationPipelineModule::UpdateModifiedTextures()
	{
		m_resourceManager->UpdateModifiedTextures();
	}

	bool DX12RasterizationPipelineModule::SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceManager->SetDescriptor(name, sourceHandle);
	}

	bool DX12RasterizationPipelineModule::SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		return m_resourceManager->SetEntityDescriptor(entity, name, sourceHandle);
	}

	bool DX12RasterizationPipelineModule::SetEntityGeometry(GameEntity* entity, const D3D12_VERTEX_BUFFER_VIEW& vertexBuffer, const D3D12_INDEX_BUFFER_VIEW& indexBuffer, UInt32 indexCount)
	{
		UInt32 slot = m_resourceManager->RegisterEntity(entity);

		if (slot == DX12MaterialResourceManager::InvalidEntityIndex)
			return false;

		if (m_entityGeometries.size() <= slot)
			m_entityGeometries.resize(slot + 1);

		m_entityGeometries[slot] = EntityGeometry{ vertexBuffer, indexBuffer, indexCount };
		return true;
	}

	void DX12RasterizationPipelineModule::BindGlobal(ID3D12GraphicsCommandList7* commandList)
	{
		commandList->SetPipelineState(m_pipelineState.Get());
		commandList->SetGraphicsRootSignature(m_rootSignature);
		commandList->IASetPrimitiveTopology(m_topology);

		m_resourceManager->BindGlobal(commandList);
	}

	bool DX12RasterizationPipelineModule::DrawSlot(ID3D12GraphicsCommandList7* commandList, UInt32 slot)
	{
		if (slot >= m_entityGeometries.size() || m_resourceManager->GetEntity(slot) == nullptr)
			return false;

		auto& geometry = m_entityGeometries[slot];

		if (geometry.indexCount == 0)
			return false;

		m_resourceManager->BindEntity(commandList, slot);
		commandList->IASetVertexBuffers(0, 1, &geometry.vertexBuffer);
		commandList->IASetIndexBuffer(&geometry.indexBuffer);
		commandList->DrawIndexedInstanced(geometry.indexCount, 1, 0, 0, 0);
		return true;
	}

	bool DX12RasterizationPipelineModule::DrawEntity(ID3D12GraphicsCommandList7* commandList, GameEntity* entity)
	{
		UInt32 slot = m_resourceManager->FindEntity(entity);

		if (slot == DX12MaterialResourceManager::InvalidEntityIndex)
			return false;

		return DrawSlot(commandList, slot);
	}

	void DX12RasterizationPipelineModule::Draw(ID3D12GraphicsCommandList7* commandList)
	{
		BindGlobal(commandList);

		// every registered entity is drawn with its own geometry
		for (UInt32 slot = 0; slot < m_resourceManager->GetEntitySlotCount(); slot++)
			DrawSlot(commandList, slot);
	}
}
