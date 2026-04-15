#include "pch.h"
#include "DX12RayTracingMaterial.h"
#include "Rendering/Material.h"
#include "HLSLRayTracingProgram.h"
#include <DX12Utilities.h>
#include "Core/Texture2D.h"
#include "DX12Texture2DController.h"

QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::DX12RayTracingMaterial(const ref<Material>& material, const ref<RayTracing::HLSLRayTracingProgram>& program)
	: m_material(material), m_program(program)
{
	program->GetRootSignature()->GetDevice(IID_PPV_ARGS(&m_device));
	auto reflectionData = program->GetReflectionData();
	auto& rootConstantBuffer = reflectionData->GetRootConstants();
	m_constantRootParameterIndex = rootConstantBuffer.rootParameterIndex;
	auto textureFields = material->GetTextureFields();
	auto valueFields = material->GetValueFields();
	auto& resourceVariableList = reflectionData->GetResourceVariables();
	m_heapValues = std::vector<HeapData>(resourceVariableList.size());
	m_constantRegisterValues = std::vector<constantBufferData>(rootConstantBuffer.blocks.size() > 0 ? rootConstantBuffer.typeDesc.Members : 0);
	
	UInt32 offset = 0;
	UInt32 textureFieldIndex = 0;
	for (UInt32 i = 0; i < reflectionData->GetRootParameterCount(); i++) {
		if (i == rootConstantBuffer.rootParameterIndex) {
			offset += (rootConstantBuffer.registerData.Num32BitValues * 4);
			continue;
		}
		
		auto rcData = reflectionData->GetResourceVariableByRootIndex(i);
		auto t = textureFields->find(rcData->name);

		if (t != textureFields->end()) {
			(*t).second.fieldIndex = textureFieldIndex;
		}

		m_heapValues[textureFieldIndex] = HeapData{
			.rootParamIndex = rcData->rootParameterIndex,
			.fieldName = rcData->name,
			.locationOffset = offset,
		};

		offset += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		textureFieldIndex++;
	}

	UInt32 internalSize = 0;
	for (auto& rootConstantBlock : rootConstantBuffer.blocks) {
		if (rootConstantBlock.isDynamic)
			internalSize += rootConstantBlock.size;
	}
	m_internalData = new Byte[internalSize];

	UInt32 internalOffset = 0;
	UInt32 locationOffset = (rootConstantBuffer.rootParameterIndex) * (UInt32)sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
	for (auto& rootConstantBlock : rootConstantBuffer.blocks) {
		if (rootConstantBlock.isDynamic) {
			for (auto& variable : rootConstantBlock.variables) {
				m_constantRegisterValues[variable.index] = constantBufferData{
					.dataLocation = m_internalData + internalOffset,
					.offset32Bits = variable.offset/4,
					.locationOffset = locationOffset + variable.offset,
					.size = rootConstantBlock.size,
				};

				internalOffset += variable.size;
			}
		}
		else {
			for (auto& variable : rootConstantBlock.variables) {
				m_constantRegisterValues[variable.index] = constantBufferData{
					.dataLocation = material->GetValueLocation(variable.name),
					.offset32Bits = variable.offset / 4,
					.locationOffset = locationOffset + variable.offset,
					.size = variable.size,
				};
			}
		}
	}
}

QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::~DX12RayTracingMaterial()
{
	delete[] m_internalData;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::GetNonGlobalResourceCounts()
{
	UInt32 count = 0;
	auto& resourceVariables = std::dynamic_pointer_cast<HLSLRayTracingProgram>(m_material->GetProgram())->GetReflectionData()->GetResourceVariables();
	
	for(auto& rc : resourceVariables) {
		if(rc.name == HLSL_RT_TLAS_SCENE_NAME ||
			rc.name == HLSL_RT_OUTPUT_TEXTURE_NAME ||
			rc.name == HLSL_CAMERA_DATA_NAME ||
			rc.name == HLSL_LIGHT_DATA_NAME) 
		{
			continue;
		}
		count++;
	}

	return count;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::LinkMaterialToDescriptorHeap(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset)
{
	auto cpuHandleStart = heap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = heap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto& resourceVariables = std::dynamic_pointer_cast<HLSLRayTracingProgram>(m_material->GetProgram())->GetReflectionData()->GetResourceVariables();
	
	cpuHandleStart.ptr += offset * incSize;
	gpuHandleStart.ptr += offset * incSize;
	UInt32 boundResourceCount = 0;

	for(auto& heapData : m_heapValues) {
		if(heapData.fieldName == HLSL_RT_TLAS_SCENE_NAME ||
			heapData.fieldName == HLSL_RT_OUTPUT_TEXTURE_NAME ||
			heapData.fieldName == HLSL_CAMERA_DATA_NAME ||
			heapData.fieldName == HLSL_LIGHT_DATA_NAME) 
		{
			continue;
		}

		heapData.cpuHandle = cpuHandleStart;
		heapData.gpuHandle = gpuHandleStart;
		cpuHandleStart.ptr += incSize;
		gpuHandleStart.ptr += incSize;
		boundResourceCount++;
	}

	return boundResourceCount;
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::SetDescriptorHandle(const std::string& fieldName, const D3D12_GPU_DESCRIPTOR_HANDLE& handle)
{
	auto it = std::find_if(
		m_heapValues.begin(),
		m_heapValues.end(),
		[&fieldName, this](const HeapData& binding) {
			auto resVar = m_program->GetReflectionData()->GetResourceVariableByRootIndex(binding.rootParamIndex);
			return resVar != nullptr && resVar->name == fieldName;
		});

	if(it != m_heapValues.end())
		(*it).gpuHandle = handle;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::LinkUserMaterials(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset)
{
	auto cpuHandleStart = heap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = heap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandleStart.ptr += offset * incSize;
	gpuHandleStart.ptr += offset * incSize;
	UInt32 boundResourceCount = 0;
	auto textureFields = m_material->GetTextureFields();

	for (auto& heapData : m_heapValues) {
		if (heapData.fieldName[0] == '_')
			continue;

		auto t = textureFields->find(heapData.fieldName);
		auto gpuTexture = std::dynamic_pointer_cast<DX12Texture2DController>((*t).second.texture->GetGPUHandle());
		
		m_device->CopyDescriptorsSimple(1, cpuHandleStart, gpuTexture->GetShaderView()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		heapData.cpuHandle = cpuHandleStart;
		heapData.gpuHandle = gpuHandleStart;
		cpuHandleStart.ptr += incSize;
		gpuHandleStart.ptr += incSize;
		boundResourceCount++;
	}

	return boundResourceCount;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::LinkInternalMaterials(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset)
{
	auto cpuHandleStart = heap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = heap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandleStart.ptr += offset * incSize;
	gpuHandleStart.ptr += offset * incSize;
	UInt32 boundResourceCount = 0;

	for (auto& heapData : m_heapValues) {
		if (heapData.fieldName[0] != '_' ||
			heapData.fieldName == HLSL_RT_TLAS_SCENE_NAME ||
			heapData.fieldName == HLSL_RT_OUTPUT_TEXTURE_NAME ||
			heapData.fieldName == HLSL_CAMERA_DATA_NAME ||
			heapData.fieldName == HLSL_LIGHT_DATA_NAME)
		{
			continue;
		}

		heapData.cpuHandle = cpuHandleStart;
		heapData.gpuHandle = gpuHandleStart;
		cpuHandleStart.ptr += incSize;
		gpuHandleStart.ptr += incSize;
		boundResourceCount++;
	}

	return boundResourceCount;
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::SetCBV(const std::string& name, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv)
{
	auto it = std::find_if(m_heapValues.begin(), m_heapValues.end(), [name](const HeapData& h) {
		return h.fieldName == name;
		});

	if(it != m_heapValues.end())
		m_device->CreateConstantBufferView(&cbv, (*it).cpuHandle);
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::SetSRV(const std::string& name, const ComPtr<ID3D12Resource2>& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv)
{
	auto it = std::find_if(m_heapValues.begin(), m_heapValues.end(), [name](const HeapData& h) {
		return h.fieldName == name;
		});

	if (it != m_heapValues.end())
		m_device->CreateShaderResourceView(resource.Get(), &srv, (*it).cpuHandle);
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::CopyVariableData(Byte* dst)
{
	auto reflectionData = m_program->GetReflectionData();
	auto& rootConstantList = reflectionData->GetRootConstants();
	auto& resourceVariableList = reflectionData->GetResourceVariables();

	m_linkedLocations.push_back(dst);

	for (UInt32 i = 0; i < reflectionData->GetRootParameterCount(); i++) {
		if (i == m_constantRootParameterIndex) {
			for (auto& cbData : m_constantRegisterValues) {
				std::memcpy(dst + cbData.locationOffset, cbData.dataLocation, cbData.size);
			}
			continue;
		}

		auto rcData = std::find_if(m_heapValues.begin(), m_heapValues.end(), [i](const HeapData& hpData) {
			return hpData.rootParamIndex == i;
			});

		D3D12_GPU_DESCRIPTOR_HANDLE* gpuPtr = (D3D12_GPU_DESCRIPTOR_HANDLE*)(dst + rcData->locationOffset);

		*gpuPtr = (*rcData).gpuHandle;
	}
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::BindParameters(ComPtr<ID3D12GraphicsCommandList7>& commandList)
{
	for (auto& rField : m_constantRegisterValues)
		commandList->SetComputeRoot32BitConstants(m_constantRootParameterIndex, rField.size/4, rField.dataLocation, rField.offset32Bits);

	for (auto& heapField : m_heapValues) {
		commandList->SetComputeRootDescriptorTable(heapField.rootParamIndex, heapField.gpuHandle);
	}
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::UpdateModifiedParameters()
{
	// Update Modified Textures
	for (auto& modified : m_material->GetModifiedTextures()) {
		auto& heapData = m_heapValues[modified->fieldIndex];
		m_device->CopyDescriptorsSimple(
			1,
			heapData.cpuHandle,
			std::dynamic_pointer_cast<DX12Texture2DController>(modified->texture->GetGPUHandle())->GetShaderView()->GetCPUDescriptorHandleForHeapStart(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	for (auto& modifiedVal : m_material->GetModifiedValues()) {
		auto& cbData = m_constantRegisterValues[modifiedVal->fieldIndex];

		for (auto g : m_linkedLocations)
			std::memcpy(g + cbData.locationOffset, cbData.dataLocation , cbData.size);
	}

	m_material->ClearModifiedTextures();
}
