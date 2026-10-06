#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12MaterialResourceManager.h"
#include <array>
#include <memory>
#include <vector>

using namespace Microsoft::WRL;

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::DX12::MeshShading {
	class DX12MeshPipelineModule {
	public:
		DX12MeshPipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState,
			ID3D12RootSignature* rootSignature, Material* material);

		~DX12MeshPipelineModule();

		DX12MeshPipelineModule(const DX12MeshPipelineModule&) = delete;
		DX12MeshPipelineModule& operator=(const DX12MeshPipelineModule&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		void Dispatch(ID3D12GraphicsCommandList7* commandList);

		bool SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y = 1, UInt32 z = 1);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return m_resourceManager->SetEntityConstant(entity, name, value);
		}

	private:
		ComPtr<ID3D12PipelineState> m_pipelineState;

		ID3D12RootSignature* m_rootSignature;
		std::unique_ptr<DX12MaterialResourceManager> m_resourceManager;

		std::vector<std::array<UInt32, 3>> m_threadGroupCounts;
	};
}
