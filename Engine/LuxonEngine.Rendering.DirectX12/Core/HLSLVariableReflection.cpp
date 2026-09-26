#include "pch.h"
#include "HLSLVariableReflection.h"
#include "HLSLShaderProgram.h"

namespace LuxonEngine::Rendering::DX12 {
	static void AddResource(ShaderVariableReflection& reflection, const D3D12_SHADER_INPUT_BIND_DESC& boundResource, ShaderStageFlags stage)
	{
		std::string name(boundResource.Name);
		ShaderResourceKind kind = ToResourceKind(boundResource);

		auto& list = kind == ShaderResourceKind::Sampler ? reflection.samplers : reflection.resources;

		auto it = std::find_if(list.begin(), list.end(), [&name](const ShaderResourceVariable& item) {
			return item.name == name;
			});

		if (it != list.end()) {
			it->stages |= stage; //the resource can exist in multiple shaders of the program
			return;
		}

		list.push_back(ShaderResourceVariable{
			.name = name,
			.kind = kind,
			.binding = boundResource.BindPoint,
			.space = boundResource.Space,
			.count = boundResource.BindCount,
			.isUnbounded = boundResource.BindCount == 0, // unbounded arrays are reported with a bind count of 0
			.isInternal = ShaderVariableReflection::IsInternalName(name),
			.stages = stage,
			});
	}

	static void AddConstants(ShaderVariableReflection& reflection, const D3D12_SHADER_INPUT_BIND_DESC& boundResource,
		ID3D12ShaderReflectionType* type, const D3D12_SHADER_TYPE_DESC& typeDesc, UInt32 bufferSize, ShaderStageFlags stage)
	{
		auto& constants = reflection.constants;

		constants.name = boundResource.Name;
		constants.size = bufferSize;
		constants.binding = boundResource.BindPoint;
		constants.space = boundResource.Space;
		constants.stages |= stage;

		UInt32 index = 0;
		UInt32 offset = 0;

		auto fillBlock = [&constants, &offset, &index, type, &typeDesc](bool internalBlock) {
			auto& blockItem = constants.blocks.emplace_back(ShaderConstantBlock{
				.offset = offset,
				.size = 0,
				.isInternal = internalBlock,
				});

			while (internalBlock == ShaderVariableReflection::IsInternalName(type->GetMemberTypeName(index))) {
				ID3D12ShaderReflectionType* memberType = type->GetMemberTypeByIndex(index);

				D3D12_SHADER_TYPE_DESC memberDesc = {};
				memberType->GetDesc(&memberDesc);

				UInt32 memberSize = (memberDesc.Rows * memberDesc.Columns * 4);

				blockItem.variables.push_back(ShaderConstantVariable{
					.name = type->GetMemberTypeName(index),
					.index = index,
					.offset = offset,
					.size = memberSize,
					.type = ToScalarType(memberDesc.Type),
					.rows = memberDesc.Rows,
					.columns = memberDesc.Columns,
					});
				blockItem.size += memberSize;

				index++;
				offset += memberSize;

				if (index >= typeDesc.Members)
					break;
			}
		};

		do {
			fillBlock(ShaderVariableReflection::IsInternalName(type->GetMemberTypeName(index)));
		} while (index < typeDesc.Members);
	}

	static D3D12_DESCRIPTOR_RANGE_TYPE ToDescriptorRangeType(ShaderResourceKind kind)
	{
		switch (kind) {
		case ShaderResourceKind::ConstantBuffer:
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		case ShaderResourceKind::RWTexture:
		case ShaderResourceKind::RWStructuredBuffer:
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		case ShaderResourceKind::Sampler:
			return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		default:
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
	}

	template<typename T>
	static void AddShaderInputBinding(ShaderVariableReflection& reflection, const D3D12_SHADER_INPUT_BIND_DESC& boundResource, T* shaderReflection, ShaderStageFlags stage)
	{
		// Constant Buffer is processed separately. Because it can be either a single buffer or collection of root constants.
		if (boundResource.Type != D3D_SIT_CBUFFER) {
			AddResource(reflection, boundResource, stage);
			return;
		}

		if (reflection.constants.name == boundResource.Name) {
			reflection.constants.stages |= stage;
			return; //the constant data can exist in multiple shaders of the program
		}

		auto resourceIt = std::find_if(reflection.resources.begin(), reflection.resources.end(),
			[&boundResource](const ShaderResourceVariable& item) { return item.name == boundResource.Name; });

		if (resourceIt != reflection.resources.end()) {
			AddResource(reflection, boundResource, stage); // only merges the stage, the resource is already added
			return;
		}

		ID3D12ShaderReflectionConstantBuffer* cb = shaderReflection->GetConstantBufferByName(boundResource.Name);

		D3D12_SHADER_BUFFER_DESC cbDesc;
		cb->GetDesc(&cbDesc);

		ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(0);

		D3D12_SHADER_VARIABLE_DESC varDesc = {};
		var->GetDesc(&varDesc);

		ID3D12ShaderReflectionType* type = var->GetType();

		D3D12_SHADER_TYPE_DESC typeDesc = {};
		type->GetDesc(&typeDesc);

		//if constant buffer size per variable is less than Matrix4, then it is considered as a holder of root constants
		if (varDesc.Size / typeDesc.Members < 32) { //TODO Find better condition for constant buffer checking if possible
			AddConstants(reflection, boundResource, type, typeDesc, cbDesc.Size, stage);
		}
		else {
			AddResource(reflection, boundResource, stage);
		}
	}
}

LuxonEngine::Rendering::ShaderStageFlags LuxonEngine::Rendering::DX12::ToShaderStage(UInt32 version)
{
	switch (D3D12_SHVER_GET_TYPE(version)) {
	case D3D12_SHVER_VERTEX_SHADER:         return ShaderStageFlags::Vertex;
	case D3D12_SHVER_GEOMETRY_SHADER:       return ShaderStageFlags::Geometry;
	case D3D12_SHVER_PIXEL_SHADER:          return ShaderStageFlags::Pixel;
	case D3D12_SHVER_COMPUTE_SHADER:        return ShaderStageFlags::Compute;
	case D3D12_SHVER_RAY_GENERATION_SHADER: return ShaderStageFlags::RayGeneration;
	case D3D12_SHVER_MISS_SHADER:           return ShaderStageFlags::Miss;
	case D3D12_SHVER_CLOSEST_HIT_SHADER:    return ShaderStageFlags::ClosestHit;
	case D3D12_SHVER_ANY_HIT_SHADER:        return ShaderStageFlags::AnyHit;
	case D3D12_SHVER_INTERSECTION_SHADER:   return ShaderStageFlags::Intersection;
	case D3D12_SHVER_CALLABLE_SHADER:       return ShaderStageFlags::Callable;
	case D3D12_SHVER_MESH_SHADER:           return ShaderStageFlags::Mesh;
	case D3D12_SHVER_AMPLIFICATION_SHADER:  return ShaderStageFlags::Amplification;
	default:                                return ShaderStageFlags::None;
	}
}

std::string LuxonEngine::Rendering::DX12::ShaderStageToString(D3D12_SHADER_VERSION_TYPE stage)
{
	switch (stage) {
	case D3D12_SHVER_VERTEX_SHADER:         return "Vertex";
	case D3D12_SHVER_GEOMETRY_SHADER:       return "Geometry";
	case D3D12_SHVER_PIXEL_SHADER:          return "Pixel";
	case D3D12_SHVER_COMPUTE_SHADER:        return "Compute";
	case D3D12_SHVER_RAY_GENERATION_SHADER: return "RayGeneration";
	case D3D12_SHVER_MISS_SHADER:           return "Miss";
	case D3D12_SHVER_CLOSEST_HIT_SHADER:    return "ClosestHit";
	case D3D12_SHVER_ANY_HIT_SHADER:        return "AnyHit";
	case D3D12_SHVER_INTERSECTION_SHADER:   return "Intersection";
	case D3D12_SHVER_CALLABLE_SHADER:       return "Callable";
	case D3D12_SHVER_MESH_SHADER:           return "Mesh";
	case D3D12_SHVER_AMPLIFICATION_SHADER:  return "Amplification";
	default:                                return "Unknown";
	}
}

LuxonEngine::Rendering::ShaderResourceKind LuxonEngine::Rendering::DX12::ToResourceKind(const D3D12_SHADER_INPUT_BIND_DESC& boundResource)
{
	// typed resources are textures unless they are declared as buffers
	bool isBuffer = boundResource.Dimension == D3D_SRV_DIMENSION_BUFFER;

	switch (boundResource.Type) {
	case D3D_SIT_CBUFFER:
		return ShaderResourceKind::ConstantBuffer;
	case D3D_SIT_TBUFFER:
	case D3D_SIT_STRUCTURED:
	case D3D_SIT_BYTEADDRESS:
		return ShaderResourceKind::StructuredBuffer;
	case D3D_SIT_TEXTURE:
		return isBuffer ? ShaderResourceKind::StructuredBuffer : ShaderResourceKind::Texture;
	case D3D_SIT_UAV_RWSTRUCTURED:
	case D3D_SIT_UAV_RWBYTEADDRESS:
	case D3D_SIT_UAV_APPEND_STRUCTURED:
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		return ShaderResourceKind::RWStructuredBuffer;
	case D3D_SIT_UAV_RWTYPED:
		return isBuffer ? ShaderResourceKind::RWStructuredBuffer : ShaderResourceKind::RWTexture;
	case D3D_SIT_SAMPLER:
		return ShaderResourceKind::Sampler;
	case D3D_SIT_RTACCELERATIONSTRUCTURE:
		return ShaderResourceKind::AccelerationStructure;
	default:
		return ShaderResourceKind::StructuredBuffer;
	}
}

LuxonEngine::Rendering::ShaderScalarType LuxonEngine::Rendering::DX12::ToScalarType(D3D_SHADER_VARIABLE_TYPE type)
{
	switch (type) {
	case D3D_SVT_BOOL:			return ShaderScalarType::Bool;
	case D3D_SVT_INT:
	case D3D_SVT_MIN16INT:		return ShaderScalarType::Int;
	case D3D_SVT_UINT:
	case D3D_SVT_UINT8:
	case D3D_SVT_MIN16UINT:		return ShaderScalarType::UInt;
	case D3D_SVT_MIN8FLOAT:
	case D3D_SVT_MIN10FLOAT:
	case D3D_SVT_MIN16FLOAT:	return ShaderScalarType::Half;
	case D3D_SVT_FLOAT:			return ShaderScalarType::Float;
	case D3D_SVT_DOUBLE:		return ShaderScalarType::Double;
	default:					return ShaderScalarType::Unknown;
	}
}

void LuxonEngine::Rendering::DX12::AddShaderReflection(ShaderVariableReflection& reflection, ID3D12ShaderReflection* shaderReflection)
{
	D3D12_SHADER_DESC desc;
	shaderReflection->GetDesc(&desc);

	ShaderStageFlags stage = ToShaderStage(desc.Version);

	for (UINT i = 0; i < desc.BoundResources; ++i) {
		D3D12_SHADER_INPUT_BIND_DESC boundResource;
		shaderReflection->GetResourceBindingDesc(i, &boundResource);
		AddShaderInputBinding(reflection, boundResource, shaderReflection, stage);
	}
}

void LuxonEngine::Rendering::DX12::AddShaderReflection(ShaderVariableReflection& reflection, ID3D12FunctionReflection* functionReflection)
{
	D3D12_FUNCTION_DESC funcDesc;
	functionReflection->GetDesc(&funcDesc);

	ShaderStageFlags stage = ToShaderStage(funcDesc.Version);

	for (UINT r = 0; r < funcDesc.BoundResources; r++) {
		D3D12_SHADER_INPUT_BIND_DESC boundResource;
		functionReflection->GetResourceBindingDesc(r, &boundResource);
		AddShaderInputBinding(reflection, boundResource, functionReflection, stage);
	}
}

void LuxonEngine::Rendering::DX12::FillRenderTargetReflection(RenderTargetReflection& reflection, ID3D12ShaderReflection* shaderReflection)
{
	D3D12_SHADER_DESC desc;
	shaderReflection->GetDesc(&desc);
	
	for(UINT i = 0; i < desc.OutputParameters; ++i) {
		D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
		shaderReflection->GetOutputParameterDesc(i, &paramDesc);

		if (paramDesc.SystemValueType == D3D_NAME_TARGET) {
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			switch (paramDesc.ComponentType) {
				case D3D_REGISTER_COMPONENT_UINT32:
					format = DXGI_FORMAT_R32_UINT;
					break;
				case D3D_REGISTER_COMPONENT_SINT32:
					format = DXGI_FORMAT_R32_SINT;
					break;
				case D3D_REGISTER_COMPONENT_FLOAT16:
					format = DXGI_FORMAT_R16_FLOAT;
					break;
				case D3D_REGISTER_COMPONENT_FLOAT32:
				case D3D_REGISTER_COMPONENT_FLOAT64:
					format = DXGI_FORMAT_R32_FLOAT;
					break;
			}
			reflection.formats.push_back(format);
		}
	}
}

ComPtr<ID3D12RootSignature> LuxonEngine::Rendering::DX12::CreateRootSignature(const ComPtr<ID3D12Device10>& device, const ShaderVariableReflection& reflection,
	D3D12_ROOT_SIGNATURE_FLAGS flag, RootParameterLayout& layout, std::string& errorStr)
{
	auto& resources = reflection.resources;

	layout = RootParameterLayout();

	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

	rootParameters.reserve(resources.size() + 1);
	ranges.reserve(resources.size()); // the ranges are referenced by the root parameters, so the storage must not grow

	// all constant variables of the program belong to the same buffer and are added as a single root constant parameter
	if (reflection.constants.blocks.empty() == false) {
		auto& constants = reflection.constants;

		layout.AddParameter(constants.name, true);

		rootParameters.push_back(D3D12_ROOT_PARAMETER{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants = D3D12_ROOT_CONSTANTS{
				.ShaderRegister = constants.binding,
				.RegisterSpace = constants.space,
				.Num32BitValues = constants.size / 4,
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			});
	}

	// samplers are added as static samplers for now with default settings
	for (auto& samplerVar : reflection.samplers) {
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
			.ShaderRegister = samplerVar.binding,
			.RegisterSpace = samplerVar.space,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			});
	}

	// all other resource variables are added as descriptor tables with single range for now
	// TODO: optimize by grouping resources of same type into single descriptor table with multiple ranges
	for (auto& resVar : resources) {
		ranges.push_back(D3D12_DESCRIPTOR_RANGE{
			.RangeType = ToDescriptorRangeType(resVar.kind),
			.NumDescriptors = resVar.isUnbounded ? UINT_MAX : resVar.count,
			.BaseShaderRegister = resVar.binding,
			.RegisterSpace = resVar.space,
			.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
			});

		layout.AddParameter(resVar.name);

		rootParameters.push_back(D3D12_ROOT_PARAMETER{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE{
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &ranges.back(),
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			});
	}

	// Create Root Signature
	D3D12_ROOT_SIGNATURE_DESC rootDesc{
		.NumParameters = (UInt32)rootParameters.size(),
		.pParameters = rootParameters.size() == 0 ? nullptr : rootParameters.data(),
		.NumStaticSamplers = (UInt32)staticSamplers.size(),
		.pStaticSamplers = staticSamplers.size() == 0 ? nullptr : staticSamplers.data(),
		.Flags = flag,
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
