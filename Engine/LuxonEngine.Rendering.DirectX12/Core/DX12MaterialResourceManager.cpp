#include "pch.h"
#include "DX12MaterialResourceManager.h"
#include "Rendering/ShaderInternalNames.h"
#include "Rendering/Material.h"
#include "Core/Texture2D.h"
#include "DX12Texture2DController.h"

namespace LuxonEngine::Rendering::DX12 {
	DX12MaterialResourceManager::DX12MaterialResourceManager(ID3D12Device10* device, Material* material, const ShaderVariableReflection& reflection,
		const RootParameterLayout& rootParameterLayout, DescriptorBindPoint bindPoint)
		:m_device(device), m_material(material), m_reflection(reflection), m_rootParameterLayout(rootParameterLayout), m_bindPoint(bindPoint)
	{
	}

	DX12MaterialResourceManager::~DX12MaterialResourceManager()
	{
		// the data of the dynamic constant blocks is owned by the entity data, the material blocks point to the material
		for (auto& entityData : m_entityDataList) {
			for (auto& constant : entityData.constants)
				delete[] constant.data;
		}
	}

	bool DX12MaterialResourceManager::Initialize(UInt32 entityCount)
	{
		m_descriptorIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		InitializeMaterialMapping();
		InitializeConstantMapping();

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = m_globalDescriptorCount + entityCount * m_entityDescriptorCount,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0,
		};

		if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap)))) {
			return false;
		}

		for (UInt32 i = 0; i < entityCount; i++) {
			m_entityDataList.push_back(CreateEntityData(nullptr));
		}

		MapHandles();
		CopyMaterialTextures();
		return true;
	}

	bool DX12MaterialResourceManager::ResizeDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC oldDesc = m_descriptorHeap->GetDesc();

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = m_globalDescriptorCount + (UInt32)m_entityDataList.size() * m_entityDescriptorCount,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0,
		};

		ComPtr<ID3D12DescriptorHeap> newHeap;

		if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeap)))) {
			return false;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE oldCpuStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE newCpuStart = newHeap->GetCPUDescriptorHandleForHeapStart();

		m_device->CopyDescriptorsSimple(oldDesc.NumDescriptors, newCpuStart, oldCpuStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_descriptorHeap = newHeap;
		MapHandles();

		return true;
	}

	void DX12MaterialResourceManager::MapHandles()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// arrays take consecutive descriptors starting from their handle
		for (auto& heapData : m_heapValues) {
			heapData.cpuHandle = cpuHandle;
			heapData.gpuHandle = gpuHandle;

			cpuHandle.ptr += heapData.descriptorCount * m_descriptorIncrementSize;
			gpuHandle.ptr += heapData.descriptorCount * m_descriptorIncrementSize;
		}

		for (auto& entityData : m_entityDataList) {
			for (auto& eHeap : entityData.resources) {
				eHeap.cpuHandle = cpuHandle;
				eHeap.gpuHandle = gpuHandle;

				cpuHandle.ptr += eHeap.descriptorCount * m_descriptorIncrementSize;
				gpuHandle.ptr += eHeap.descriptorCount * m_descriptorIncrementSize;
			}
		}
	}

	void DX12MaterialResourceManager::InitializeMaterialMapping()
	{
		m_globalDescriptorCount = 0;
		m_entityDescriptorCount = 0;

		for (auto& resourceVariable : m_reflection.resources) {
			if (resourceVariable.isUnbounded)
				continue;

			if (ShaderInternalNames::IsPerEntity(resourceVariable.name)) {
				m_entityHeapLayout[resourceVariable.name.c_str()] = (int)m_entityDataTemplate.resources.size(); // index in EntityDynamicData::resources
				m_entityDataTemplate.resources.push_back(HeapData{
					.rootParamIndex = m_rootParameterLayout.GetRootParameterIndex(resourceVariable.name),
					.name = resourceVariable.name.c_str(),
					.descriptorCount = resourceVariable.count,
					.cpuHandle = 0,
					.gpuHandle = 0,
					});
				m_entityDescriptorCount += resourceVariable.count;
				continue;
			}

			HeapData heapData{
				.rootParamIndex = m_rootParameterLayout.GetRootParameterIndex(resourceVariable.name),
				.name = resourceVariable.name.c_str(),
				.descriptorCount = resourceVariable.count,
				.heapOffset = m_globalDescriptorCount,
				.cpuHandle = 0,
				.gpuHandle = 0,
			};

			m_globalDescriptorCount += resourceVariable.count;
			m_heapValues.push_back(heapData);
		}

		// lookup table from the texture field index of the material to the heap value of this pipeline
		auto textureFields = m_material->GetTextureFields();
		m_textureHeapIndices = std::vector<UInt32>(textureFields->size(), ShaderVariableReflection::InvalidIndex);

		for (UInt32 i = 0; i < m_heapValues.size(); i++) {
			auto it = textureFields->find(m_heapValues[i].name);

			if (it != textureFields->end())
				m_textureHeapIndices[it->second.fieldIndex] = i;
		}
	}

	void DX12MaterialResourceManager::InitializeConstantMapping()
	{
		auto& constants = m_reflection.constants;
		m_rootConstantsParamIndex = m_rootParameterLayout.GetConstantDataRootIndex();

		for (auto& block : constants.blocks) {
			if (block.variables.empty())
				continue;

			ConstantData constantData{
				.name = block.variables[0].name.c_str(),
				.offset = block.offset / 4,
				.size = block.size / 4,
			};

			// material block: the variables are consecutive in the material data, starting from the first one
			if (block.isInternal == false) {
				constantData.data = m_material->GetValueLocation(block.variables[0].name);
				m_rootConstants.push_back(constantData);
				continue;
			}

			// dynamic block: the data is allocated per entity when the entity data is created
			UInt32 blockIndex = (UInt32)m_entityDataTemplate.constants.size();

			for (auto& variable : block.variables) {
				m_entityConstantLayout[variable.name] = ConstantVariableLocation{
					.blockIndex = blockIndex,
					.offset = variable.offset - block.offset,
					.size = variable.size,
				};
			}

			m_entityDataTemplate.constants.push_back(constantData);
		}
	}

	DX12MaterialResourceManager::EntityDynamicData DX12MaterialResourceManager::CreateEntityData(GameEntity* entity) const
	{
		EntityDynamicData entityData = m_entityDataTemplate;
		entityData.entity = entity;

		for (auto& constant : entityData.constants)
			constant.data = new Byte[constant.size * 4]();

		return entityData;
	}

	DX12MaterialResourceManager::EntityDynamicData* DX12MaterialResourceManager::GetOrRegisterEntity(GameEntity* entity)
	{
		auto entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [entity](const EntityDynamicData& entityData) {
			return entityData.entity == entity;
			});

		if (entityIT != m_entityDataList.end())
			return &(*entityIT);

		// take a free slot reserved by Initialize
		entityIT = std::find_if(m_entityDataList.begin(), m_entityDataList.end(), [](const EntityDynamicData& entityData) {
			return entityData.entity == nullptr;
			});

		if (entityIT != m_entityDataList.end()) {
			entityIT->entity = entity;
			return &(*entityIT);
		}

		m_entityDataList.push_back(CreateEntityData(entity));

		if (ResizeDescriptorHeap() == false)
			return nullptr;

		return &m_entityDataList.back();
	}

	UInt32 DX12MaterialResourceManager::RegisterEntity(GameEntity* entity)
	{
		if (entity == nullptr)
			return InvalidEntityIndex;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return InvalidEntityIndex;

		return (UInt32)(entityData - m_entityDataList.data());
	}

	UInt32 DX12MaterialResourceManager::FindEntity(GameEntity* entity) const
	{
		if (entity == nullptr)
			return InvalidEntityIndex;

		for (UInt32 slot = 0; slot < m_entityDataList.size(); slot++) {
			if (m_entityDataList[slot].entity == entity)
				return slot;
		}

		return InvalidEntityIndex;
	}

	bool DX12MaterialResourceManager::SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		if (ShaderInternalNames::IsGlobal(name) == false)
			return false;

		auto it = std::find_if(m_heapValues.begin(), m_heapValues.end(), [name](const HeapData& heapData) {
			return strcmp(heapData.name, name) == 0;
			});

		if (it == m_heapValues.end()) // the program does not use this global variable
			return false;

		m_device->CopyDescriptorsSimple(it->descriptorCount, it->cpuHandle, sourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return true;
	}

	bool DX12MaterialResourceManager::SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		if (entity == nullptr || ShaderInternalNames::IsPerEntity(name) == false)
			return false;

		auto layoutIT = m_entityHeapLayout.find(name);

		if (layoutIT == m_entityHeapLayout.end())
			return false;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return false;

		// a newly registered entity is mapped by the heap resize, so its handle is valid here
		auto& heapData = entityData->resources[layoutIT->second];
		m_device->CopyDescriptorsSimple(heapData.descriptorCount, heapData.cpuHandle, sourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return true;
	}

	bool DX12MaterialResourceManager::SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size)
	{
		if (entity == nullptr)
			return false;

		auto layoutIT = m_entityConstantLayout.find(name);

		if (layoutIT == m_entityConstantLayout.end() || layoutIT->second.size != size)
			return false;

		EntityDynamicData* entityData = GetOrRegisterEntity(entity);

		if (entityData == nullptr)
			return false;

		auto& location = layoutIT->second;
		memcpy(entityData->constants[location.blockIndex].data + location.offset, data, size);
		return true;
	}

	void DX12MaterialResourceManager::BindGlobal(ID3D12GraphicsCommandList7* commandList)
	{
		UpdateModifiedTextures();

		ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		// global and material bindings are shared by all the entities
		for (auto& heapData : m_heapValues)
			SetRootDescriptorTable(commandList, heapData.rootParamIndex, heapData.gpuHandle);

		for (auto& constant : m_rootConstants) {
			if (constant.data != nullptr)
				SetRootConstants(commandList, constant);
		}
	}

	void DX12MaterialResourceManager::BindEntity(ID3D12GraphicsCommandList7* commandList, UInt32 slot)
	{
		auto& entityData = m_entityDataList[slot];

		for (auto& heapData : entityData.resources)
			SetRootDescriptorTable(commandList, heapData.rootParamIndex, heapData.gpuHandle);

		for (auto& constant : entityData.constants)
			SetRootConstants(commandList, constant);
	}

	void DX12MaterialResourceManager::SetRootDescriptorTable(ID3D12GraphicsCommandList7* commandList, UInt32 rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle) const
	{
		if (m_bindPoint == DescriptorBindPoint::Compute)
			commandList->SetComputeRootDescriptorTable(rootParamIndex, handle);
		else
			commandList->SetGraphicsRootDescriptorTable(rootParamIndex, handle);
	}

	void DX12MaterialResourceManager::SetRootConstants(ID3D12GraphicsCommandList7* commandList, const ConstantData& constant) const
	{
		if (m_bindPoint == DescriptorBindPoint::Compute)
			commandList->SetComputeRoot32BitConstants(m_rootConstantsParamIndex, constant.size, constant.data, constant.offset);
		else
			commandList->SetGraphicsRoot32BitConstants(m_rootConstantsParamIndex, constant.size, constant.data, constant.offset);
	}

	void DX12MaterialResourceManager::CopyMaterialTextures()
	{
		if (m_material == nullptr)
			return;

		for (auto& [name, textureData] : *m_material->GetTextureFields()) {
			if (textureData.texture != nullptr)
				CopyTextureDescriptor(textureData);
		}
	}

	void DX12MaterialResourceManager::UpdateModifiedTextures()
	{
		if (m_material == nullptr || m_descriptorHeap == nullptr)
			return;

		for (auto* textureData : m_material->GetModifiedTextures())
			CopyTextureDescriptor(*textureData);

		// only the textures are cleared, the modified values are applied separately
		m_material->ClearTextures();
	}

	void DX12MaterialResourceManager::CopyTextureDescriptor(const MaterialTextureData& textureData)
	{
		UInt32 heapIndex = m_textureHeapIndices[textureData.fieldIndex];

		if (heapIndex == ShaderVariableReflection::InvalidIndex)
			return;

		D3D12_CPU_DESCRIPTOR_HANDLE destination = m_heapValues[heapIndex].cpuHandle;

		// the texture is removed from the material. a null descriptor reads zero instead of the previous texture
		if (textureData.texture == nullptr) {
			D3D12_SHADER_RESOURCE_VIEW_DESC nullViewDesc{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D = D3D12_TEX2D_SRV{ .MipLevels = 1 },
			};

			m_device->CreateShaderResourceView(nullptr, &nullViewDesc, destination);
			return;
		}

		auto dx12Texture = std::dynamic_pointer_cast<DX12Texture2DController>(textureData.texture->GetGPUHandle());

		if (dx12Texture == nullptr) // the texture is not uploaded to the GPU yet
			return;

		m_device->CopyDescriptorsSimple(1, destination, dx12Texture->GetShaderView()->GetCPUDescriptorHandleForHeapStart(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}
