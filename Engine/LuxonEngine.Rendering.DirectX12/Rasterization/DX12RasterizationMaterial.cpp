#include "pch.h"
#include "DX12RasterizationMaterial.h"
#include "Rendering/Material.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "Core/Texture2D.h"
#include "DX12Texture2DController.h"
#include <DX12Utilities.h>

QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::DX12RasterizationMaterial(const ref<Material>& material, const ref<DX12::Rasterization::HLSLRasterizationProgram>& program)
	:m_material(material), m_program(program) 
{
	program->GetRootSignature()->GetDevice(IID_PPV_ARGS(&m_device));
	auto reflectionData = program->GetReflectionData();
	auto& rootConstantBuffer = reflectionData->GetRootConstants();
	for (auto& rootConstantBlock : rootConstantBuffer.blocks) {
		if(rootConstantBlock.isDynamic)
			continue;

		m_constantRegisterValues.push_back(constantBufferData{
			.rootParamIndex = rootConstantBuffer.rootParameterIndex,
			.location = material->GetValueLocation(rootConstantBlock.variables[0].name),//-cbData.rootConstants[0].variableDesc.StartOffset / 4,
			.size = rootConstantBlock.size / 4,
			});
	}

	auto textureFields = material->GetTextureFields();
	m_heapValues = std::vector<HeapData>(textureFields->size());
	auto& resourceVariableList = reflectionData->GetResourceVariables();

	for (auto& resVar : *textureFields) {
		ref<DX12Texture2DController> dx12Texture = std::dynamic_pointer_cast<DX12Texture2DController>(resVar.second.texture->GetGPUHandle());
		auto it = std::find_if(
			resourceVariableList.begin(),
			resourceVariableList.end(),
			[&resVar](const ResourceVariableData& binding) {
				return binding.name == resVar.first;
			});

		m_heapValues[resVar.second.fieldIndex] = HeapData{
			.rootParamIndex = (*it).rootParameterIndex,
			.name = resVar.first,
			.originalCpuHandle = dx12Texture->GetShaderView()->GetCPUDescriptorHandleForHeapStart(),
		};
	}

	// Find Special Resource root parameter indices
	for (auto& resVar : resourceVariableList) {
		if (resVar.name == HLSL_OBJECT_TRANSFORM_DATA_NAME) {
			m_transformRootIndex = resVar.rootParameterIndex;
		}
		else if (resVar.name == HLSL_CAMERA_DATA_NAME) {
			m_cameraRootIndex = resVar.rootParameterIndex;
		}
		else if (resVar.name == HLSL_LIGHT_DATA_NAME) {
			m_lightRootIndex = resVar.rootParameterIndex;
		}
		else if (resVar.name[0] == '_') {
			m_heapValues.push_back(HeapData{
			.rootParamIndex = resVar.rootParameterIndex,
			.name = resVar.name,
			});
		}
	}
}

void QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::BindDescriptorToResources(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UInt32 offset)
{
    auto incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += offset * incrementSize;
    gpuHandle.ptr += offset * incrementSize;

	auto textureList = m_material->GetTextureFields();

	// Bind material-specific textures to the descriptor handles
    for (auto& heap : m_heapValues) {
		if (heap.name[0] == '_')
			continue;

        heap.cpuHandle = cpuHandle;
        heap.gpuHandle = gpuHandle;

        cpuHandle.ptr += incrementSize;
        gpuHandle.ptr += incrementSize;
    }
}

void QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::BindParameters(ComPtr<ID3D12GraphicsCommandList7>& commandList)
{
	// Update Modified Textures
    for(auto& modified : m_material->GetModifiedTextures()) {
        auto& heapData = m_heapValues[modified->fieldIndex];
        m_device->CopyDescriptorsSimple(
            1,
            heapData.cpuHandle,
            std::dynamic_pointer_cast<DX12Texture2DController>(modified->texture->GetGPUHandle())->GetShaderView()->GetCPUDescriptorHandleForHeapStart(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	m_material->ClearModifiedTextures();

	// Sett Root Constants
    for(auto& constant : m_constantRegisterValues) {
        commandList->SetGraphicsRoot32BitConstants(
            constant.rootParamIndex,
            constant.size,
            constant.location,
            0);
	}

	// Set Descriptor Parameters
    for(auto& heap : m_heapValues) {
        commandList->SetGraphicsRootDescriptorTable(
            heap.rootParamIndex,
            heap.gpuHandle);
	}
}

void QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::BindTransformDescriptor(ComPtr<ID3D12GraphicsCommandList7>& commandList, const D3D12_GPU_DESCRIPTOR_HANDLE& transformHeapHandle)
{
    if(m_transformRootIndex == -1)
        return;
	commandList->SetGraphicsRootDescriptorTable(m_transformRootIndex, transformHeapHandle);
}

void QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::BindCameraDescriptor(ComPtr<ID3D12GraphicsCommandList7>& commandList, const D3D12_GPU_DESCRIPTOR_HANDLE& cameraHeapHandle)
{
    if(m_cameraRootIndex == -1)
        return;
	commandList->SetGraphicsRootDescriptorTable(m_cameraRootIndex, cameraHeapHandle);
}

void QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::BindLightDescriptor(ComPtr<ID3D12GraphicsCommandList7>& commandList, const D3D12_GPU_DESCRIPTOR_HANDLE& lightHeapHandle)
{
    if(m_lightRootIndex == -1)
        return;
	commandList->SetGraphicsRootDescriptorTable(m_lightRootIndex, lightHeapHandle);
}

void QuantumEngine::Rendering::DX12::Rasterization::DX12RasterizationMaterial::SetDescriptorHandles(const std::string& fieldName, const D3D12_GPU_DESCRIPTOR_HANDLE& handle)
{
	auto it = std::find_if(
		m_heapValues.begin(),
		m_heapValues.end(),
		[&fieldName, this](const HeapData& binding) {
			auto resVar = m_program->GetReflectionData()->GetResourceVariableByRootIndex(binding.rootParamIndex);
			return resVar != nullptr && resVar->name == fieldName;
		});

	if (it != m_heapValues.end())
		(*it).gpuHandle = handle;
}
