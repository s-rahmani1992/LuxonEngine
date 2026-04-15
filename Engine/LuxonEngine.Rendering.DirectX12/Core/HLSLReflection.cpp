#include "pch.h"
#include "HLSLReflection.h"

void QuantumEngine::Rendering::DX12::HLSLReflection::AddShaderReflection(ID3D12ShaderReflection* shaderReflection)
{
    D3D12_SHADER_DESC desc;
    shaderReflection->GetDesc(&desc);

    for (UINT i = 0; i < desc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC boundResource;
        shaderReflection->GetResourceBindingDesc(i, &boundResource);
		AddShaderInputBindings(boundResource, shaderReflection);
    }
}

void QuantumEngine::Rendering::DX12::HLSLReflection::AddShaderReflection(ID3D12FunctionReflection* functionReflection)
{
    D3D12_FUNCTION_DESC funcDesc;
    functionReflection->GetDesc(&funcDesc);

    for (int r = 0; r < funcDesc.BoundResources; r++) {
        D3D12_SHADER_INPUT_BIND_DESC boundResource;
        functionReflection->GetResourceBindingDesc(r, &boundResource);
        AddShaderInputBindings(boundResource, functionReflection);
    }
}

ComPtr<ID3D12RootSignature> QuantumEngine::Rendering::DX12::HLSLReflection::CreateRootSignature(const ComPtr<ID3D12Device10>& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::string& errorStr)
{
    std::vector<D3D12_ROOT_PARAMETER> rootParameters(m_resourceVariables.size() + (m_rootConstantBuffer.blocks.size() > 0 ? 1 : 0));
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges(m_resourceVariables.size());
    UInt8 rangeIndex = 0;

    // all root constants belonging to the same buffer are added as a single root constant variable
    
    if (m_rootConstantBuffer.blocks.size() > 0) {
        rootParameters[m_rootConstantBuffer.rootParameterIndex] = D3D12_ROOT_PARAMETER{
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
        .Constants = m_rootConstantBuffer.registerData,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };
    }
    
    // samplers are added as static samplers for now with default settings
    for (auto& samplerVar : m_samplerVariables) {
        staticSamplers.push_back(D3D12_STATIC_SAMPLER_DESC{
            .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .MipLODBias = 0.0f,
            .MaxAnisotropy = 1,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            .MinLOD = 0.0f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = samplerVar.resourceData.BindPoint,
            .RegisterSpace = samplerVar.resourceData.Space,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            });
    }

    // all other resource variables are added as descriptor tables with single range for now
    // TODO: optimize by grouping resources of same type into single descriptor table with multiple ranges
    for (auto& resVar : m_resourceVariables) {
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
        switch (resVar.resourceData.Type) {
        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            break;
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            break;
        case D3D_SIT_CBUFFER:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            break;
        default:
            break;
        }

        ranges[rangeIndex] = D3D12_DESCRIPTOR_RANGE{
        .RangeType = rangeType,
        .NumDescriptors = 1,
        .BaseShaderRegister = resVar.resourceData.BindPoint,
        .RegisterSpace = resVar.resourceData.Space,
        .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
        };

        rootParameters[resVar.rootParameterIndex] = D3D12_ROOT_PARAMETER{
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE{
            .NumDescriptorRanges = 1,
            .pDescriptorRanges = ranges.data() + rangeIndex,
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };

        rangeIndex++;
    }

    // Create Root Signature
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = flag;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{
        .NumParameters = (UInt32)rootParameters.size(),
        .pParameters = rootParameters.size() == 0 ? nullptr : rootParameters.data(),
        .NumStaticSamplers = (UInt32)staticSamplers.size(),
        .pStaticSamplers = staticSamplers.size() == 0 ? nullptr : staticSamplers.data(),
        .Flags = rootSignatureFlag,
    };

    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> rootSignatureByteCode;
    ComPtr<ID3D12RootSignature> rootSignature;

    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureByteCode, &error))) {

        errorStr = std::string((char*)error->GetBufferPointer(), error->GetBufferSize());
        return nullptr;
    }

    if (FAILED(device->CreateRootSignature(0, rootSignatureByteCode->GetBufferPointer()
        , rootSignatureByteCode->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
		errorStr = "Failed to create root signature.";
        return nullptr;
    }

    return rootSignature;
}

QuantumEngine::Rendering::MaterialReflection QuantumEngine::Rendering::DX12::HLSLReflection::CreateMaterialReflection(bool rtField)
{
    UInt32 fieldIndex = 0;
    MaterialReflection reflectionData;

    // Value Fields are from Root Constants

    for (auto& rootConstBlock : m_rootConstantBuffer.blocks) {
        if (rootConstBlock.isDynamic == '_') {// Skip internal root constants
            fieldIndex++;
            continue;
        }

        for (auto& rootVar : rootConstBlock.variables) {
            UInt32 varSize = (rootVar.variableDesc.Rows * rootVar.variableDesc.Columns * 4);
            MaterialValueFieldInfo valueFieldInfo{
                .name = rootVar.name,
                .fieldIndex = fieldIndex,
                .size = varSize,
            };
            reflectionData.valueFields.push_back(valueFieldInfo);
            fieldIndex++;
        }
    }

    // Texture Fields
    
    for (auto& resVar : m_resourceVariables) {
        if (resVar.name[0] == '_') {// Skip internal variables
            fieldIndex++;
            continue;
        }

        // Only textures are considered for material texture fields
        if (resVar.resourceData.Type == D3D_SIT_TEXTURE) {
            MaterialTextureFieldInfo textureFieldInfo{
                .name = resVar.name,
                .fieldIndex = rtField ? resVar.rootParameterIndex : fieldIndex,
            };
            reflectionData.textureFields.push_back(textureFieldInfo);
            fieldIndex++;
        }
    }
    return reflectionData;
}

UInt32 QuantumEngine::Rendering::DX12::HLSLReflection::GetRootParameterIndexByName(const std::string& name) const
{
    if (m_rootConstantBuffer.name == name)
        return m_rootConstantBuffer.rootParameterIndex;
    
    for (const auto& rv : m_resourceVariables) {
        if (rv.name == name) {
            return rv.rootParameterIndex;
        }
    }

    for (const auto& sv : m_samplerVariables) {
        if (sv.name == name) {
            return sv.rootParameterIndex;
        }
    }

    return -1; // invalid index
}

QuantumEngine::Rendering::DX12::ResourceVariableData* QuantumEngine::Rendering::DX12::HLSLReflection::GetResourceVariableByRootIndex(UInt32 rootParameterIndex)
{
    for (auto& rv : m_resourceVariables) {
        if (rv.rootParameterIndex == rootParameterIndex) {
            return &rv;
        }
    }

    return nullptr;
}

QuantumEngine::Rendering::DX12::RootConstantBufferData* QuantumEngine::Rendering::DX12::HLSLReflection::GetRootConstantByRootIndex(UInt32 rootParameterIndex)
{
    if (m_rootConstantBuffer.rootParameterIndex == rootParameterIndex)
        return &m_rootConstantBuffer;

    return nullptr;
}

UInt32 QuantumEngine::Rendering::DX12::HLSLReflection::GetTotalVariableSize()
{
    UInt32 variableSize = 0;

    for (auto& rootConstantBlock : m_rootConstantBuffer.blocks) {
        variableSize += (rootConstantBlock.size);
    }

    variableSize += (sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * m_resourceVariables.size());

    return variableSize;
    return UInt32();
}

QuantumEngine::Rendering::DX12::RootConstantVariableData* QuantumEngine::Rendering::DX12::HLSLReflection::GetRootConstantVariableByName(const std::string& name)
{
    for (auto& rootConstantBlock : m_rootConstantBuffer.blocks) {
        for (auto& rootVariable : rootConstantBlock.variables) {
            if (rootVariable.name == name)
                return &rootVariable;
        }
    }
    return nullptr;
}
