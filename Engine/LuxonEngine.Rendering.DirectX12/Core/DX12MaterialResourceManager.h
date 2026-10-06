#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "Rendering/ShaderReflection.h"
#include "HLSLShaderProgram.h"
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

namespace LuxonEngine::Rendering::DX12 {
	enum class DescriptorBindPoint {
		Graphics,
		Compute,
	};

	class DX12MaterialResourceManager {
	public:
		static constexpr UInt32 InvalidEntityIndex = ShaderVariableReflection::InvalidIndex;

		DX12MaterialResourceManager(ID3D12Device10* device, Material* material, const ShaderVariableReflection& reflection,
			const RootParameterLayout& rootParameterLayout, DescriptorBindPoint bindPoint);

		~DX12MaterialResourceManager();

		DX12MaterialResourceManager(const DX12MaterialResourceManager&) = delete;
		DX12MaterialResourceManager& operator=(const DX12MaterialResourceManager&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return SetEntityConstantData(entity, name, &value, sizeof(T));
		}

		bool SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size);

		UInt32 RegisterEntity(GameEntity* entity);

		UInt32 FindEntity(GameEntity* entity) const;

		inline UInt32 GetEntitySlotCount() const { return (UInt32)m_entityDataList.size(); }

		inline GameEntity* GetEntity(UInt32 slot) const { return m_entityDataList[slot].entity; }

		void BindGlobal(ID3D12GraphicsCommandList7* commandList);

		void BindEntity(ID3D12GraphicsCommandList7* commandList, UInt32 slot);

	private:
		struct HeapData {
			UInt32 rootParamIndex;
			const char* name; // points into the reflection of the program
			UInt32 descriptorCount = 1;
			UInt32 heapOffset;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
		};

		struct ConstantData {
			const char* name; // first variable of the block, points into the reflection of the program
			UInt32 offset; // destination offset in 32 bit values
			UInt32 size; // number of 32 bit values
			Byte* data = nullptr; // material data for material blocks, allocated per entity for dynamic blocks
		};

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

		void CopyMaterialTextures();

		void CopyTextureDescriptor(const MaterialTextureData& textureData);

		void SetRootDescriptorTable(ID3D12GraphicsCommandList7* commandList, UInt32 rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

		void SetRootConstants(ID3D12GraphicsCommandList7* commandList, const ConstantData& constant) const;

		ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

		ID3D12Device10* m_device;
		Material* m_material;
		const ShaderVariableReflection& m_reflection;
		const RootParameterLayout& m_rootParameterLayout;
		DescriptorBindPoint m_bindPoint;

		UInt32 m_descriptorIncrementSize = 0;
		UInt32 m_globalDescriptorCount = 0;
		UInt32 m_entityDescriptorCount = 0;

		std::vector<HeapData> m_heapValues;
		std::vector<UInt32> m_textureHeapIndices; // texture field index of the material -> index in m_heapValues
		std::map<std::string_view, int> m_entityHeapLayout; // per entity variables. the handles are set per entity in EntityDynamicData::resources

		UInt32 m_rootConstantsParamIndex = ShaderVariableReflection::InvalidIndex; // root parameter shared by all the root constant blocks
		std::vector<ConstantData> m_rootConstants; // material root constant blocks
		std::map<std::string_view, ConstantVariableLocation> m_entityConstantLayout; // dynamic constant variables of an entity

		std::vector<EntityDynamicData> m_entityDataList;
		EntityDynamicData m_entityDataTemplate;
	};
}
