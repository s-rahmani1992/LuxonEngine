#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12MaterialResourceManager.h"
#include <memory>
#include <vector>

using namespace Microsoft::WRL;

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::DX12::Rasterization {
	class DX12RasterizationPipelineModule {
	public:
		DX12RasterizationPipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState,
			ID3D12RootSignature* rootSignature, Material* material, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		~DX12RasterizationPipelineModule();

		DX12RasterizationPipelineModule(const DX12RasterizationPipelineModule&) = delete;
		DX12RasterizationPipelineModule& operator=(const DX12RasterizationPipelineModule&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityGeometry(GameEntity* entity, const D3D12_VERTEX_BUFFER_VIEW& vertexBuffer, const D3D12_INDEX_BUFFER_VIEW& indexBuffer, UInt32 indexCount);

		/// <summary>
		/// Sets the pipeline state, root signature, topology and binds the global and material resources shared by all the entities.
		/// the pipeline binds its own descriptor heap, so the heap of the other pipelines must be set again after it
		/// </summary>
		void BindGlobal(ID3D12GraphicsCommandList7* commandList);

		/// <summary>
		/// Binds the descriptors, constants and geometry of the entity and draws it. BindGlobal must be called before.
		/// </summary>
		/// <returns>false if the entity is not registered or has no geometry</returns>
		bool DrawEntity(ID3D12GraphicsCommandList7* commandList, GameEntity* entity);

		/// <summary>
		/// BindGlobal followed by DrawEntity for every registered entity. the render targets, viewport and scissor must be set by the caller
		/// </summary>
		void Draw(ID3D12GraphicsCommandList7* commandList);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return m_resourceManager->SetEntityConstant(entity, name, value);
		}

	private:
		bool DrawSlot(ID3D12GraphicsCommandList7* commandList, UInt32 slot);

		struct EntityGeometry {
			D3D12_VERTEX_BUFFER_VIEW vertexBuffer = {};
			D3D12_INDEX_BUFFER_VIEW indexBuffer = {};
			UInt32 indexCount = 0;
		};

		ComPtr<ID3D12PipelineState> m_pipelineState;

		ID3D12RootSignature* m_rootSignature;
		D3D12_PRIMITIVE_TOPOLOGY m_topology; // list topology matching the topology type of the pipeline state
		std::unique_ptr<DX12MaterialResourceManager> m_resourceManager;

		std::vector<EntityGeometry> m_entityGeometries; // per entity slot of the resource manager
	};
}
