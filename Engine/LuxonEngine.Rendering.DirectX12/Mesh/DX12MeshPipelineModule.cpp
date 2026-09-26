#include "pch.h"
#include "DX12MeshPipelineModule.h"

namespace LuxonEngine::Rendering::DX12::MeshShading {
	DX12MeshPipelineModule::DX12MeshPipelineModule(const ComPtr<ID3D12Device10>& device, const ComPtr<ID3D12PipelineState>& pipelineState,
		const ComPtr<ID3D12RootSignature>& rootSignature, const Material* material)
		:m_device(device), m_pipelineState(pipelineState), m_rootSignature(rootSignature), m_material(material)
	{

	}
}
