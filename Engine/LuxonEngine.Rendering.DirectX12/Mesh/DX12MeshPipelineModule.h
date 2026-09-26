#pragma once
#include "pch.h"
#include "BasicTypes.h"

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::DX12::MeshShading {
	class DX12MeshPipelineModule {
	public:
		DX12MeshPipelineModule(const ComPtr<ID3D12Device10>& device, const ComPtr<ID3D12PipelineState>& pipelineState,
			const ComPtr<ID3D12RootSignature>& rootSignature, const Material* material);

		inline ComPtr<ID3D12Device10> GetDevice() const { return m_device; }
		inline ComPtr<ID3D12PipelineState> GetPipelineState() const { return m_pipelineState; }
		inline ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_rootSignature; }

		inline const Material* GetMaterial() const { return m_material; }

	private:
		ComPtr<ID3D12Device10> m_device;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		const Material* m_material;
	};
}
