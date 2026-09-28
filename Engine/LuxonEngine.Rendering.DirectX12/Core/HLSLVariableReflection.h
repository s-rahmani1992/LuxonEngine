#pragma once

#include "pch.h"
#include "BasicTypes.h"
#include <string>
#include <unordered_map>
#include "HLSLShaderProgram.h"
#include <Rendering/ShaderReflection.h>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12 {
	class RootParameterLayout;

	ShaderStageFlags ToShaderStage(UInt32 version);

	std::string ShaderStageToString(D3D12_SHADER_VERSION_TYPE stage);

	ShaderResourceKind ToResourceKind(const D3D12_SHADER_INPUT_BIND_DESC& boundResource);

	ShaderScalarType ToScalarType(D3D_SHADER_VARIABLE_TYPE type);

	void AddShaderReflection(ShaderVariableReflection& reflection, ID3D12ShaderReflection* shaderReflection);

	void AddShaderReflection(ShaderVariableReflection& reflection, ID3D12FunctionReflection* functionReflection);

	void FillRenderTargetReflection(RenderTargetReflection& reflection, ID3D12ShaderReflection* shaderReflection);

	void FillThreadGroupReflection(ShaderThreadGroupReflection& reflection, ID3D12ShaderReflection* shaderReflection);

	ComPtr<ID3D12RootSignature> CreateRootSignature(const ComPtr<ID3D12Device10>& device, const ShaderVariableReflection& reflection,
		D3D12_ROOT_SIGNATURE_FLAGS flag, RootParameterLayout& layout, std::string& error);
}
