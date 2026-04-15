#pragma once
#include "../pch.h"
#include <vector>
#include <BasicTypes.h>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	class Transform;
	struct Matrix4;

	namespace Rendering::DX12 {
		class DX12MeshController;
	}
}

namespace QuantumEngine::Rendering::DX12::RayTracing {
	struct EntityBLASDesc {
		UInt32 hitGroupIndex;
		ref<DX12MeshController> mesh;
		ref<Transform> transform;
	};

	class RTAccelarationStructure {
	public:
		bool Initialize(const ComPtr<ID3D12GraphicsCommandList7>& commandList, const std::vector<EntityBLASDesc>& entities);
		void UpdateTransforms(const ComPtr<ID3D12GraphicsCommandList7>& commandList, Matrix4& viewMatrix);
		inline ComPtr<ID3D12DescriptorHeap> GetDescriptor() { return m_tlasHeap; }
		inline ComPtr<ID3D12Resource2> GetResource() { return m_topLevelAccelerationStructure; }
	private:
		struct EntityBLAS {
			ref<Transform> transform; 
			UInt32 hitIndex;
			ComPtr<ID3D12Resource2> BLASResource;
		};
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
		std::vector<EntityBLAS> m_entities;
		ComPtr<ID3D12Resource2> m_topLevelAccelerationStructure;
		ComPtr<ID3D12DescriptorHeap> m_tlasHeap;
		ComPtr<ID3D12Resource2> m_tlasUpload;
		ComPtr<ID3D12Resource2> m_rtScratchBuffer;
		std::vector<ComPtr<ID3D12Resource2>> scratchBuffers; //TODO : Remove the scratch buffers from here, they should be temporary
	};
}