#pragma once
#include "pch.h"
#include <vector>
#include <memory>
#include "BasicTypes.h"

namespace QuantumEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;
	struct DX12MeshRendererGPUData;
	class DX12MeshController;

	namespace Rasterization {
		class DX12RasterizationMaterial;
	}

	class DX12GameEntityPipelineModule
	{
	public:
		bool Initialize(const ComPtr<ID3D12Device10>& device, const DX12MeshRendererGPUData& meshRendererData, DXGI_FORMAT depthFormat);
		void Render(ComPtr<ID3D12GraphicsCommandList7>& commandList, D3D12_GPU_DESCRIPTOR_HANDLE camHandle, D3D12_GPU_DESCRIPTOR_HANDLE lightHandle);

	private:
		ref<Rasterization::DX12RasterizationMaterial> m_material;
		D3D12_GPU_DESCRIPTOR_HANDLE m_transformHeapHandle;
		ref<DX12MeshController> m_meshController;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipeline;
	};
}
