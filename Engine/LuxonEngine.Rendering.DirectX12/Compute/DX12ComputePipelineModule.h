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

namespace LuxonEngine::Rendering::DX12::Compute {
	class DX12ComputePipelineModule {
	public:
		DX12ComputePipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState,
			ID3D12RootSignature* rootSignature, Material* material);

		~DX12ComputePipelineModule();

		DX12ComputePipelineModule(const DX12ComputePipelineModule&) = delete;
		DX12ComputePipelineModule& operator=(const DX12ComputePipelineModule&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y = 1, UInt32 z = 1);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return m_resourceManager->SetEntityConstant(entity, name, value);
		}

		void BindGlobal(ID3D12GraphicsCommandList7* commandList);

		bool DispatchEntity(ID3D12GraphicsCommandList7* commandList, GameEntity* entity);

		void Dispatch(ID3D12GraphicsCommandList7* commandList);

	private:
		bool DispatchSlot(ID3D12GraphicsCommandList7* commandList, UInt32 slot);

		ComPtr<ID3D12PipelineState> m_pipelineState;

		ID3D12RootSignature* m_rootSignature;
		std::unique_ptr<DX12MaterialResourceManager> m_resourceManager;

		std::vector<std::array<UInt32, 3>> m_threadGroupCounts; // per entity slot of the resource manager
	};
}
