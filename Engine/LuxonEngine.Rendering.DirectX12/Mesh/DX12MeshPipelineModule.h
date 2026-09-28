#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "Rendering/ShaderReflection.h"
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::WRL;

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
	struct MaterialTextureData;
}

namespace LuxonEngine::Rendering::DX12::MeshShading {
	class HLSLMeshProgram;

	class DX12MeshPipelineModule {
	public:
		DX12MeshPipelineModule(ID3D12Device10* device, const ComPtr<ID3D12PipelineState>& pipelineState,
			ID3D12RootSignature* rootSignature, Material* material);

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return SetEntityConstantData(entity, name, &value, sizeof(T));
		}

		~DX12MeshPipelineModule();

		DX12MeshPipelineModule(const DX12MeshPipelineModule&) = delete;
		DX12MeshPipelineModule& operator=(const DX12MeshPipelineModule&) = delete;

	private:
		struct HeapData {
			UInt32 rootParamIndex;
			const char* name; // points into the reflection of the program
			UInt32 descriptorCount = 1;
			UInt32 heapOffset;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
		};

		// a root constant block of the variable reflection
		struct ConstantData {
			const char* name; // first variable of the block, points into the reflection of the program
			UInt32 offset; // destination offset in 32 bit values
			UInt32 size; // number of 32 bit values
			Byte* data = nullptr; // material data for material blocks, allocated per entity for dynamic blocks
		};

		// location of a variable of the dynamic constant blocks
		struct ConstantVariableLocation {
			UInt32 blockIndex; // index in EntityDynamicData::constants
			UInt32 offset; // byte offset in the data of the block
			UInt32 size; // size in bytes
		};

		struct EntityDynamicData {
			GameEntity* entity = nullptr;
			std::vector<HeapData> resources;
			std::vector<ConstantData> constants;
		};

		bool ResizeDescriptorHeap();

		void MapHandles();

		void InitializeMaterialMapping();

		void InitializeConstantMapping();

		EntityDynamicData CreateEntityData(GameEntity* entity) const;

		EntityDynamicData* GetOrRegisterEntity(GameEntity* entity);

		bool SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size);

		void CopyMaterialTextures();

		void CopyTextureDescriptor(const MaterialTextureData& textureData);

		ComPtr<ID3D12DescriptorHeap> m_pipelineDescriptorHeap;
		ComPtr<ID3D12PipelineState> m_pipelineState;

		ID3D12Device10* m_device;
		ID3D12RootSignature* m_rootSignature;
		Material* m_material;
		HLSLMeshProgram* m_meshProgram;

		UInt32 m_descriptorIncrementSize = 0;
		UInt32 m_globalDescriptorCount = 0;
		UInt32 m_entityDescriptorCount = 0;

		std::vector<HeapData> m_heapValues;
		std::vector<UInt32> m_textureHeapIndices; // texture field index of the material -> index in m_heapValues
		std::map<const char*, int> m_entityHeapLayout; // per entity variables. the handles are set per entity in m_entityHeapData

		UInt32 m_rootConstantsParamIndex = ShaderVariableReflection::InvalidIndex; // root parameter shared by all the root constant blocks
		std::vector<ConstantData> m_rootConstants; // material root constant blocks
		std::map<std::string_view, ConstantVariableLocation> m_entityConstantLayout; // dynamic constant variables of an entity

		std::vector<EntityDynamicData> m_entityDataList;
		EntityDynamicData m_entityDataTemplate;
	};
}
