#pragma once
#include "pch.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <BasicTypes.h>

using namespace Microsoft::WRL;

namespace LuxonEngine {
	class Transform;

	namespace Rendering::DX12 {
		class DX12MeshController;
	}
}

namespace LuxonEngine::Rendering::DX12::RayTracing {
	class RTSceneAccelerationStructure {
	public:
		struct Instance {
			ComPtr<ID3D12Resource2> bottomLevel; // null for an inactive instance. it has the instance mask 0, so no ray hits it
			ref<Transform> transform;
			UInt32 instanceId = 0;
			UInt32 hitGroupIndex = 0;
		};

		RTSceneAccelerationStructure(ID3D12Device10* device);

		RTSceneAccelerationStructure(const RTSceneAccelerationStructure&) = delete;
		RTSceneAccelerationStructure& operator=(const RTSceneAccelerationStructure&) = delete;

		ComPtr<ID3D12Resource2> GetOrBuildBottomLevel(ID3D12GraphicsCommandList7* commandList, const ref<DX12MeshController>& mesh);

		void ReleaseUnusedBottomLevels(const std::unordered_set<DX12MeshController*>& usedMeshes);

		bool Rebuild(ID3D12GraphicsCommandList7* commandList, const std::vector<Instance>& instances);

		void UpdateTransforms(ID3D12GraphicsCommandList7* commandList);

		void ReleaseScratchBuffers();

		inline ID3D12Resource2* GetResource() const { return m_topLevel.Get(); }
		inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_topLevel != nullptr ? m_topLevel->GetGPUVirtualAddress() : 0; }

	private:
		bool EnsureInstanceBuffer(UInt32 instanceCount);
		void FillInstanceDescs();
		void BuildTopLevel(ID3D12GraphicsCommandList7* commandList, bool update);

		ID3D12Device10* m_device;

		struct BottomLevel {
			ref<DX12MeshController> mesh; // keeps the mesh alive while the structure is cached
			ComPtr<ID3D12Resource2> resource;
		};

		std::unordered_map<DX12MeshController*, BottomLevel> m_bottomLevels;
		std::vector<ComPtr<ID3D12Resource2>> m_scratchBuffers; // scratch of the bottom level builds of the current command list

		std::vector<Instance> m_instances;
		ComPtr<ID3D12Resource2> m_topLevel;
		ComPtr<ID3D12Resource2> m_topLevelScratch;
		ComPtr<ID3D12Resource2> m_instanceBuffer; // upload buffer. it is mapped for its whole life
		D3D12_RAYTRACING_INSTANCE_DESC* m_instanceDescs = nullptr;
		UInt32 m_instanceCapacity = 0;
	};
}
