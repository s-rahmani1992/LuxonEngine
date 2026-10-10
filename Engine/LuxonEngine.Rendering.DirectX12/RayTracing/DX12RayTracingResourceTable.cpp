#include "pch.h"
#include "DX12RayTracingResourceTable.h"
#include "HLSLRayTracingProgram.h"
#include "Rendering/ShaderInternalNames.h"
#include "Rendering/Material.h"
#include "Core/Texture2D.h"
#include "DX12Texture2DController.h"
#include "Core/Logger.h"
#include <algorithm>

namespace LuxonEngine::Rendering::DX12::RayTracing {
	DX12RayTracingResourceTable::DX12RayTracingResourceTable(ID3D12Device10* device)
		:m_device(device)
	{
		m_descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	DX12RayTracingResourceTable::~DX12RayTracingResourceTable()
	{
		if (m_shaderTable != nullptr)
			m_shaderTable->Unmap(0, nullptr);
	}

	bool DX12RayTracingResourceTable::Initialize(Material* globalMaterial, const std::vector<EntityMaterial>& entities)
	{
		if (AddBlock(globalMaterial) == InvalidIndex)
			return false;

		for (auto& entityMaterial : entities) {
			if (entityMaterial.entity == nullptr || entityMaterial.material == nullptr)
				continue;

			UInt32 blockIndex = AddBlock(entityMaterial.material);

			if (blockIndex == InvalidIndex)
				return false;

			m_entityIndices[entityMaterial.entity] = (UInt32)m_entityDataList.size();
			m_entityDataList.push_back(EntityData{
				.entity = entityMaterial.entity,
				.block = blockIndex,
				.resources = m_blocks[blockIndex].entityTemplate,
				});
		}

		if (CreateDescriptorHeap() == false)
			return false;

		MapHandles();

		for (auto& block : m_blocks) {
			for (auto& [name, textureData] : *block.material->GetTextureFields()) {
				if (textureData.texture != nullptr)
					CopyMaterialTexture(block, textureData);
			}
		}

		return true;
	}

	UInt32 DX12RayTracingResourceTable::AddBlock(Material* material)
	{
		if (material == nullptr)
			return InvalidIndex;

		auto blockIt = m_blockIndices.find(material);

		if (blockIt != m_blockIndices.end())
			return blockIt->second;

		MaterialBlock block;
		block.material = material;
		block.program = dynamic_cast<HLSLRayTracingProgram*>(material->GetProgram().get());

		if (block.program == nullptr || block.program->GetRootSignature() == nullptr)
			return InvalidIndex;

		auto& rootParameterLayout = block.program->GetRootParameterLayout();

		for (auto& resource : block.program->GetVariableReflection().resources) {
			if (resource.isUnbounded)
				continue;

			HeapData heapData{
				.rootParamIndex = rootParameterLayout.GetRootParameterIndex(resource.name),
				.name = resource.name.c_str(),
				.descriptorCount = resource.count,
			};

			if (ShaderInternalNames::IsGlobal(resource.name)) {
				// shared by all the programs. the first program that uses the name decides the number of descriptors
				if (m_globalLayout.contains(resource.name) == false) {
					m_globalLayout[heapData.name] = (int)m_globalHeapValues.size();
					m_globalHeapValues.push_back(heapData);
				}

				// the record of the program points to the shared descriptor
				block.globalBindings.emplace_back(heapData.rootParamIndex, (UInt32)m_globalLayout[heapData.name]);
			}
			else if (ShaderInternalNames::IsPerEntity(resource.name)) {
				block.entityLayout[heapData.name] = (int)block.entityTemplate.size();
				block.entityTemplate.push_back(heapData);
			}
			else {
				block.heapValues.push_back(heapData);
			}
		}

		// lookup table from the texture field index of the material to the heap value of the program
		auto textureFields = material->GetTextureFields();
		block.textureHeapIndices = std::vector<UInt32>(textureFields->size(), InvalidIndex);

		for (UInt32 i = 0; i < block.heapValues.size(); i++) {
			auto it = textureFields->find(block.heapValues[i].name);

			if (it != textureFields->end())
				block.textureHeapIndices[it->second.fieldIndex] = i;
		}

		// the values of the material are root constants. the location of every value in the shader record is known from the layout of the program
		UInt32 constantsRootIndex = rootParameterLayout.GetConstantDataRootIndex();

		block.constantsRootIndex = constantsRootIndex;

		if (constantsRootIndex != InvalidIndex) {
			UInt32 constantsOffset = block.program->GetRecordOffset(constantsRootIndex);
			auto valueFields = material->GetValueFields();
			UInt32 valueFieldCount = 0;

			for (auto& [name, valueField] : *valueFields)
				valueFieldCount = std::max(valueFieldCount, valueField.fieldIndex + 1);

			block.valueData.assign(valueFieldCount, ValueData{});

			for (auto& constantBlock : block.program->GetVariableReflection().constants.blocks) {
				for (auto& variable : constantBlock.variables) {
					if (variable.name == INTERNAL_RT_MISS_INDEX_NAME) {
						block.missIndexRecordOffset = constantsOffset + variable.offset;
						block.missIndexConstantOffset = variable.offset;
					}

					auto valueIt = valueFields->find(variable.name);

					if (constantBlock.isInternal || valueIt == valueFields->end())
						continue;

					block.valueData[valueIt->second.fieldIndex] = ValueData{
						.source = valueIt->second.data,
						.recordOffset = constantsOffset + variable.offset,
						.constantOffset = variable.offset,
						.size = std::min(variable.size, valueIt->second.size),
					};
				}
			}
		}

		UInt32 blockIndex = (UInt32)m_blocks.size();
		m_blockIndices[material] = blockIndex;
		m_blocks.push_back(std::move(block));
		return blockIndex;
	}

	bool DX12RayTracingResourceTable::CreateDescriptorHeap()
	{
		UInt32 descriptorCount = 0;

		for (auto& heapData : m_globalHeapValues)
			descriptorCount += heapData.descriptorCount;

		for (auto& block : m_blocks) {
			for (auto& heapData : block.heapValues)
				descriptorCount += heapData.descriptorCount;
		}

		for (auto& entityData : m_entityDataList) {
			for (auto& heapData : entityData.resources)
				descriptorCount += heapData.descriptorCount;
		}

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = descriptorCount > 0 ? descriptorCount : 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0,
		};

		return SUCCEEDED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap)));
	}

	void DX12RayTracingResourceTable::MapHandles()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// arrays take consecutive descriptors starting from their handle
		auto mapList = [&](std::vector<HeapData>& heapValues) {
			for (auto& heapData : heapValues) {
				heapData.cpuHandle = cpuHandle;
				heapData.gpuHandle = gpuHandle;

				cpuHandle.ptr += heapData.descriptorCount * m_descriptorIncrementSize;
				gpuHandle.ptr += heapData.descriptorCount * m_descriptorIncrementSize;
			}
		};

		mapList(m_globalHeapValues);

		for (auto& block : m_blocks)
			mapList(block.heapValues);

		for (auto& entityData : m_entityDataList)
			mapList(entityData.resources);
	}

	bool DX12RayTracingResourceTable::SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		if (ShaderInternalNames::IsGlobal(name) == false)
			return false;

		// the acceleration structure is owned by the pipeline
		if (strcmp(name, INTERNAL_RT_TLAS_SCENE_NAME) == 0)
			return false;

		auto layoutIt = m_globalLayout.find(name);

		if (layoutIt == m_globalLayout.end()) // none of the programs uses this global variable
			return false;

		auto& heapData = m_globalHeapValues[layoutIt->second];
		m_device->CopyDescriptorsSimple(heapData.descriptorCount, heapData.cpuHandle, sourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return true;
	}

	bool DX12RayTracingResourceTable::SetAccelerationStructure(D3D12_GPU_VIRTUAL_ADDRESS address)
	{
		auto layoutIt = m_globalLayout.find(INTERNAL_RT_TLAS_SCENE_NAME);

		if (layoutIt == m_globalLayout.end()) // none of the programs uses the acceleration structure
			return false;

		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{
			.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.RaytracingAccelerationStructure = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV{ .Location = address },
		};

		m_device->CreateShaderResourceView(nullptr, &viewDesc, m_globalHeapValues[layoutIt->second].cpuHandle);
		return true;
	}

	bool DX12RayTracingResourceTable::SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
	{
		if (ShaderInternalNames::IsPerEntity(name) == false)
			return false;

		auto entityIt = m_entityIndices.find(entity);

		if (entityIt == m_entityIndices.end())
			return false;

		auto& entityData = m_entityDataList[entityIt->second];
		auto& layout = m_blocks[entityData.block].entityLayout;
		auto layoutIt = layout.find(name);

		if (layoutIt == layout.end()) // the program of the entity does not use this variable
			return false;

		auto& heapData = entityData.resources[layoutIt->second];
		m_device->CopyDescriptorsSimple(heapData.descriptorCount, heapData.cpuHandle, sourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return true;
	}

	void DX12RayTracingResourceTable::CopyMaterialTexture(MaterialBlock& block, const MaterialTextureData& textureData)
	{
		UInt32 heapIndex = block.textureHeapIndices[textureData.fieldIndex];

		if (heapIndex == InvalidIndex)
			return;

		D3D12_CPU_DESCRIPTOR_HANDLE destination = block.heapValues[heapIndex].cpuHandle;

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

	void DX12RayTracingResourceTable::UpdateModifiedTextures()
	{
		for (auto& block : m_blocks) {
			for (auto* textureData : block.material->GetModifiedTextures())
				CopyMaterialTexture(block, *textureData);

			// only the textures are cleared, the modified values are applied separately
			block.material->ClearTextures();
		}
	}

	// Shader binding table

	namespace {
		constexpr UInt32 AlignUp(UInt32 value, UInt32 alignment)
		{
			return (value + alignment - 1) / alignment * alignment;
		}
	}

	bool DX12RayTracingResourceTable::EnsureShaderTableBuffer(UInt64 size)
	{
		if (m_shaderTable != nullptr && size <= m_shaderTableCapacity)
			return true;

		// grows geometrically, so adding entities one by one does not create the buffer every time
		UInt64 capacity = std::max<UInt64>(size, m_shaderTableCapacity * 2);

		D3D12_RESOURCE_DESC bufferDesc{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = capacity,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = { .Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};

		D3D12_HEAP_PROPERTIES uploadHeapProps{ .Type = D3D12_HEAP_TYPE_UPLOAD };
		ComPtr<ID3D12Resource2> buffer;

		if (FAILED(m_device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer))))
			return false;

		void* mapped = nullptr;
		D3D12_RANGE noRead{ 0, 0 };

		if (FAILED(buffer->Map(0, &noRead, &mapped)))
			return false;

		// the GPU must not use the previous buffer anymore
		if (m_shaderTable != nullptr)
			m_shaderTable->Unmap(0, nullptr);

		m_shaderTable = buffer;
		m_shaderTableData = static_cast<Byte*>(mapped);
		m_shaderTableCapacity = capacity;
		return true;
	}

	Byte* DX12RayTracingResourceTable::GetHitRecord(UInt32 entityIndex) const
	{
		return m_shaderTableData + m_hitOffset + (size_t)(m_firstEntityHitRecord + entityIndex) * m_hitStride;
	}

	Byte* DX12RayTracingResourceTable::GetMissRecord(UInt32 missRecord) const
	{
		return m_shaderTableData + m_missOffset + (size_t)missRecord * m_missStride;
	}

	void DX12RayTracingResourceTable::WriteRecordArguments(const MaterialBlock& block, const EntityData* entity, Byte* arguments) const
	{
		auto writeHandle = [&](UInt32 rootParamIndex, const D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle) {
			if (rootParamIndex != InvalidIndex)
				std::memcpy(arguments + block.program->GetRecordOffset(rootParamIndex), &gpuHandle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
			};

		for (auto& [rootParamIndex, globalIndex] : block.globalBindings)
			writeHandle(rootParamIndex, m_globalHeapValues[globalIndex].gpuHandle);

		for (auto& heapData : block.heapValues)
			writeHandle(heapData.rootParamIndex, heapData.gpuHandle);

		if (entity != nullptr) {
			for (auto& heapData : entity->resources)
				writeHandle(heapData.rootParamIndex, heapData.gpuHandle);
		}

		for (auto& valueData : block.valueData) {
			if (valueData.source != nullptr)
				std::memcpy(arguments + valueData.recordOffset, valueData.source, valueData.size);
		}

		if (block.missIndexRecordOffset != InvalidIndex)
			std::memcpy(arguments + block.missIndexRecordOffset, &block.missIndex, sizeof(UInt32));
	}

	bool DX12RayTracingResourceTable::BuildShaderTable(ID3D12StateObject* stateObject)
	{
		ComPtr<ID3D12StateObjectProperties> stateProperties;

		if (FAILED(stateObject->QueryInterface(IID_PPV_ARGS(&stateProperties))))
			return false;

		constexpr UInt32 identifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		HLSLRayTracingProgram* globalProgram = m_blocks[0].program;

		// Miss records. the global program is the first one, every other program with a miss shader gets one record
		// that uses the arguments of its first material. the index of the record is the miss index of all the materials of the program
		std::vector<UInt32> missBlocks; // InvalidIndex for the global program
		std::unordered_map<HLSLRayTracingProgram*, UInt32> missIndices;
		UInt32 missArgumentSize = 0;

		if (globalProgram->HasMissStage())
			missBlocks.push_back(InvalidIndex);

		for (UInt32 blockIndex = 1; blockIndex < m_blocks.size(); blockIndex++) {
			auto& block = m_blocks[blockIndex];
			block.missRecord = InvalidIndex;
			block.missIndex = 0;

			if (block.program->HasMissStage() == false)
				continue;

			auto missIt = missIndices.find(block.program);

			if (missIt == missIndices.end()) {
				missIt = missIndices.emplace(block.program, (UInt32)missBlocks.size()).first;
				block.missRecord = missIt->second;
				missBlocks.push_back(blockIndex);
				missArgumentSize = std::max(missArgumentSize, block.program->GetRecordArgumentSize());
			}

			block.missIndex = missIt->second;
		}

		m_missCount = (UInt32)missBlocks.size();
		m_missStride = m_missCount > 0 ? AlignUp(identifierSize + missArgumentSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) : 0;

		// Hit records. the global program has a record without arguments, then every entity has one
		UInt32 hitArgumentSize = 0;

		for (auto& entityData : m_entityDataList)
			hitArgumentSize = std::max(hitArgumentSize, m_blocks[entityData.block].program->GetRecordArgumentSize());

		m_firstEntityHitRecord = globalProgram->HasHitGroup() ? 1 : 0;
		m_hitCount = m_firstEntityHitRecord + (UInt32)m_entityDataList.size();
		m_hitStride = m_hitCount > 0 ? AlignUp(identifierSize + hitArgumentSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) : 0;

		// Layout. every section starts at the table alignment. a section without records has no size, so no stride is used as a divisor
		m_rayGenSize = AlignUp(identifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		m_missOffset = AlignUp(m_rayGenSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		m_hitOffset = AlignUp(m_missOffset + m_missCount * m_missStride, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		UInt64 totalSize = (UInt64)m_hitOffset + (UInt64)m_hitCount * m_hitStride;

		if (EnsureShaderTableBuffer(totalSize) == false)
			return false;

		std::memset(m_shaderTableData, 0, totalSize);

		auto writeIdentifier = [&stateProperties](Byte* record, const wchar_t* exportName) {
			const void* identifier = stateProperties->GetShaderIdentifier(exportName);

			if (identifier == nullptr) {
				Logger::LogError("The ray tracing state object has no shader identifier for an export of the shader table");
				return;
			}

			std::memcpy(record, identifier, identifierSize);
			};

		writeIdentifier(m_shaderTableData, globalProgram->GetRayGenExportName().c_str());

		for (UInt32 i = 0; i < m_missCount; i++) {
			Byte* record = GetMissRecord(i);

			if (missBlocks[i] == InvalidIndex) {
				writeIdentifier(record, globalProgram->GetMissExportName().c_str());
				continue;
			}

			auto& block = m_blocks[missBlocks[i]];
			writeIdentifier(record, block.program->GetMissExportName().c_str());
			WriteRecordArguments(block, nullptr, record + identifierSize);
		}

		if (globalProgram->HasHitGroup())
			writeIdentifier(m_shaderTableData + m_hitOffset, globalProgram->GetHitGroupDesc()->HitGroupExport);

		for (UInt32 i = 0; i < m_entityDataList.size(); i++) {
			auto& entityData = m_entityDataList[i];
			auto& block = m_blocks[entityData.block];
			Byte* record = GetHitRecord(i);

			// a record without identifier is a null hit group
			if (block.program->HasHitGroup())
				writeIdentifier(record, block.program->GetHitGroupDesc()->HitGroupExport);

			WriteRecordArguments(block, &entityData, record + identifierSize);
		}

		return true;
	}

	void DX12RayTracingResourceTable::BindGlobal(ID3D12GraphicsCommandList7* commandList)
	{
		ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		auto& block = m_blocks[0];

		for (auto& [rootParamIndex, globalIndex] : block.globalBindings) {
			if (rootParamIndex != InvalidIndex)
				commandList->SetComputeRootDescriptorTable(rootParamIndex, m_globalHeapValues[globalIndex].gpuHandle);
		}

		for (auto& heapData : block.heapValues) {
			if (heapData.rootParamIndex != InvalidIndex)
				commandList->SetComputeRootDescriptorTable(heapData.rootParamIndex, heapData.gpuHandle);
		}

		if (block.constantsRootIndex == InvalidIndex)
			return;

		// every value is a part of the root constants. the offset is in 32 bit values
		for (auto& valueData : block.valueData) {
			if (valueData.source != nullptr)
				commandList->SetComputeRoot32BitConstants(block.constantsRootIndex, valueData.size / 4, valueData.source, valueData.constantOffset / 4);
		}

		if (block.missIndexConstantOffset != InvalidIndex)
			commandList->SetComputeRoot32BitConstants(block.constantsRootIndex, 1, &block.missIndex, block.missIndexConstantOffset / 4);
	}

	void DX12RayTracingResourceTable::FillDispatchRaysDesc(D3D12_DISPATCH_RAYS_DESC& desc) const
	{
		D3D12_GPU_VIRTUAL_ADDRESS startAddress = m_shaderTable->GetGPUVirtualAddress();

		desc.RayGenerationShaderRecord = D3D12_GPU_VIRTUAL_ADDRESS_RANGE{ startAddress, m_rayGenSize };
		desc.MissShaderTable = D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
			m_missCount > 0 ? startAddress + m_missOffset : 0, (UInt64)m_missCount * m_missStride, m_missStride };
		desc.HitGroupTable = D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
			m_hitCount > 0 ? startAddress + m_hitOffset : 0, (UInt64)m_hitCount * m_hitStride, m_hitStride };
		desc.CallableShaderTable = D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{ 0, 0, 0 };
	}

	void DX12RayTracingResourceTable::UpdateModifiedValues()
	{
		// the values of the global program are root constants of the command list, not of a shader record
		for (UInt32 blockIndex = 1; blockIndex < m_blocks.size(); blockIndex++) {
			auto& block = m_blocks[blockIndex];
			auto& modifiedValues = block.material->GetModifiedValues();

			if (modifiedValues.empty())
				continue;

			// the shader records that use the material: the records of its entities and its miss record
			std::vector<Byte*> arguments;

			for (UInt32 i = 0; i < m_entityDataList.size(); i++) {
				if (m_entityDataList[i].block == blockIndex)
					arguments.push_back(GetHitRecord(i) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			}

			if (block.missRecord != InvalidIndex)
				arguments.push_back(GetMissRecord(block.missRecord) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

			for (auto* modifiedValue : modifiedValues) {
				if (modifiedValue->fieldIndex >= block.valueData.size() || block.valueData[modifiedValue->fieldIndex].source == nullptr)
					continue;

				auto& valueData = block.valueData[modifiedValue->fieldIndex];

				for (Byte* argument : arguments)
					std::memcpy(argument + valueData.recordOffset, valueData.source, valueData.size);
			}

			block.material->ClearModifiedValues();
		}
	}
}
