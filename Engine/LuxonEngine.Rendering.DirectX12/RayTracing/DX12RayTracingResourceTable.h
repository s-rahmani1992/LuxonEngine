#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "Rendering/ShaderReflection.h"
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace Microsoft::WRL;

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
	struct MaterialTextureData;
}

namespace LuxonEngine::Rendering::DX12::RayTracing {
	class HLSLRayTracingProgram;

	class DX12RayTracingResourceTable {
	public:
		static constexpr UInt32 InvalidIndex = ShaderVariableReflection::InvalidIndex;

		struct EntityMaterial {
			GameEntity* entity;
			Material* material;
		};

		DX12RayTracingResourceTable(ID3D12Device10* device);
		~DX12RayTracingResourceTable();

		DX12RayTracingResourceTable(const DX12RayTracingResourceTable&) = delete;
		DX12RayTracingResourceTable& operator=(const DX12RayTracingResourceTable&) = delete;

		bool Initialize(Material* globalMaterial, const std::vector<EntityMaterial>& entities);

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		void UpdateModifiedTextures();

		void UpdateModifiedValues();

		bool BuildShaderTable(ID3D12StateObject* stateObject);

		void BindGlobal(ID3D12GraphicsCommandList7* commandList);

		void FillDispatchRaysDesc(D3D12_DISPATCH_RAYS_DESC& desc) const;

		bool SetAccelerationStructure(D3D12_GPU_VIRTUAL_ADDRESS address);

		inline UInt32 GetFirstEntityHitRecord() const { return m_firstEntityHitRecord; }

		inline ID3D12DescriptorHeap* GetHeap() const { return m_descriptorHeap.Get(); }

	private:
		struct HeapData {
			UInt32 rootParamIndex;
			const char* name; // points into the reflection of the program
			UInt32 descriptorCount = 1;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
		};

		struct ValueData {
			const Byte* source = nullptr; // the value in the material
			UInt32 recordOffset = 0; // location of the value in the arguments of a shader record
			UInt32 constantOffset = 0; // location of the value in the root constants
			UInt32 size = 0;
		};

		struct MaterialBlock {
			Material* material = nullptr;
			HLSLRayTracingProgram* program = nullptr;
			std::vector<std::pair<UInt32, UInt32>> globalBindings; // root parameter index, index in m_globalHeapValues of the global variables of the program
			std::vector<HeapData> heapValues; // material variables
			std::vector<UInt32> textureHeapIndices; // texture field index of the material -> index in heapValues
			std::vector<HeapData> entityTemplate; // entity variables, copied for every entity of the material
			std::map<std::string_view, int> entityLayout; // entity variable name -> index in entityTemplate
			std::vector<ValueData> valueData; // value field index of the material -> location in the shader record
			UInt32 constantsRootIndex = InvalidIndex; // root parameter of the root constants
			UInt32 missIndexRecordOffset = InvalidIndex; // location of the internal miss index variable in the shader record
			UInt32 missIndexConstantOffset = InvalidIndex; // location of the internal miss index variable in the root constants
			UInt32 missIndex = 0; // index of the miss shader of the program in the miss table
			UInt32 missRecord = InvalidIndex; // index of the miss record that gets the arguments of this block
		};

		struct EntityData {
			GameEntity* entity = nullptr;
			UInt32 block = InvalidIndex;
			std::vector<HeapData> resources; // handles of the entity variables
		};

		UInt32 AddBlock(Material* material);
		bool CreateDescriptorHeap();
		void MapHandles();
		void CopyMaterialTexture(MaterialBlock& block, const MaterialTextureData& textureData);

		bool EnsureShaderTableBuffer(UInt64 size);
		void WriteRecordArguments(const MaterialBlock& block, const EntityData* entity, Byte* arguments) const;
		Byte* GetHitRecord(UInt32 entityIndex) const;
		Byte* GetMissRecord(UInt32 missRecord) const;

		ID3D12Device10* m_device;
		ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
		UInt32 m_descriptorIncrementSize = 0;

		std::vector<HeapData> m_globalHeapValues; // one per global variable name of all the programs
		std::map<std::string_view, int> m_globalLayout; // global variable name -> index in m_globalHeapValues

		std::vector<MaterialBlock> m_blocks; // block 0 is the global program
		std::unordered_map<Material*, UInt32> m_blockIndices;
		std::vector<EntityData> m_entityDataList; // the index of an entity is its hit record, after the hit record of the global program
		std::unordered_map<GameEntity*, UInt32> m_entityIndices;

		// One buffer for the three sections: ray generation, miss, hit. The hit section is the last one, because it grows with the entities
		ComPtr<ID3D12Resource2> m_shaderTable; // upload buffer. it is mapped for its whole life
		Byte* m_shaderTableData = nullptr;
		UInt64 m_shaderTableCapacity = 0;
		UInt32 m_rayGenSize = 0;
		UInt32 m_missOffset = 0;
		UInt32 m_missStride = 0;
		UInt32 m_missCount = 0;
		UInt32 m_hitOffset = 0;
		UInt32 m_hitStride = 0;
		UInt32 m_hitCount = 0;
		UInt32 m_firstEntityHitRecord = 0; // 1 if the global program has a hit group
	};
}
