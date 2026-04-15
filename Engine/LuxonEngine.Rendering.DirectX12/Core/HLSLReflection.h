#pragma once

#include "pch.h"
#include "BasicTypes.h"
#include <string>
#include <vector>
#include <Core/Matrix4.h>
#include <Rendering/Material.h>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	struct RootConstantVariableData {
		std::string name;
		UInt32 index;
		UInt32 size;
		UInt32 offset;
		D3D12_SHADER_TYPE_DESC variableDesc;
	};

	struct RootConstantBlockData {
		UInt32 offset;
		UInt32 size;
		bool isDynamic;
		std::vector<RootConstantVariableData> variables;
	};

	struct RootConstantBufferData {
		UInt32 rootParameterIndex;
		std::string name;
		D3D12_SHADER_INPUT_BIND_DESC resourceData;
		D3D12_ROOT_CONSTANTS registerData;
		D3D12_SHADER_TYPE_DESC typeDesc;
		std::vector<RootConstantBlockData> blocks;
	};

	struct ResourceVariableData {
		UInt32 rootParameterIndex;
		std::string name;
		D3D12_SHADER_INPUT_BIND_DESC resourceData;
	};

	class HLSLReflection {
	public:
		HLSLReflection() = default;

		/// <summary>
		/// Merges the reflection data from the shader into this reflection data
		/// </summary>
		/// <param name="shaderReflection">Raw DX12 Reflection</param>
		void AddShaderReflection(ID3D12ShaderReflection* shaderReflection);

		/// <summary>
		/// Merges the reflection data from the ray tracing function into this reflection data
		/// </summary>
		/// <param name="functionReflection"></param>
		void AddShaderReflection(ID3D12FunctionReflection* functionReflection);

		/// <summary>
		/// Creates the DirectX 12 root signature from this reflection data
		/// </summary>
		/// <param name="device">DX12 Device for creating root signature</param>
		/// <param name="flag">root signature flag</param>
		/// <param name="error">output string containing error if creation fails</param>
		/// <returns></returns>
		ComPtr<ID3D12RootSignature> CreateRootSignature(const ComPtr<ID3D12Device10>& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::string& error);
		
		/// <summary>
		/// Creates Reflection for creating User material
		/// </summary>
		/// <param name="rtField">determine if the destination is for ray tracing material</param>
		/// <returns></returns>
		MaterialReflection CreateMaterialReflection(bool rtField);

		/// <summary>
		/// Gets index of root parameter
		/// </summary>
		/// <param name="name">name of the field</param>
		/// <returns>returns -1 if the parameter not found</returns>
		UInt32 GetRootParameterIndexByName(const std::string& name) const;

		/// <summary>
		/// Gets list of root constant data
		/// </summary>
		/// <returns></returns>
		inline RootConstantBufferData& GetRootConstants() { return m_rootConstantBuffer; }
		
		/// <summary>
		/// Gets list of resource variable data
		/// </summary>
		/// <returns></returns>
		inline std::vector<ResourceVariableData>& GetResourceVariables() { return m_resourceVariables; }
		
		/// <summary>
		/// Gets list of sampler variable data
		/// </summary>
		/// <returns></returns>
		inline std::vector<ResourceVariableData>& GetSamplerVariables() { return m_samplerVariables; }
	
		/// <summary>
		/// Gets total number of root parameters
		/// </summary>
		/// <returns></returns>
		inline UInt32 GetRootParameterCount() const { return m_rootparameterCount; }

		/// <summary>
		/// Gets pointer to Resource Element by root parameter index
		/// </summary>
		/// <param name="rootParameterIndex"></param>
		/// <returns></returns>
		ResourceVariableData* GetResourceVariableByRootIndex(UInt32 rootParameterIndex);

		/// <summary>
		/// Gets pointer to the root constant element by root parameter index
		/// </summary>
		/// <param name="rootParameterIndex"></param>
		/// <returns></returns>
		RootConstantBufferData* GetRootConstantByRootIndex(UInt32 rootParameterIndex);

		/// <summary>
		/// Gets the sum of the parameters' sizes in bytes
		/// </summary>
		/// <returns></returns>
		UInt32 GetTotalVariableSize();

		/// <summary>
		/// Gets the Root Constant Variable Data by the name. if it does not exist, returns null
		/// </summary>
		/// <param name="name">variable name</param>
		/// <returns></returns>
		RootConstantVariableData* GetRootConstantVariableByName(const std::string& name);

	private:

		template<typename T>
		void AddShaderInputBindings(const D3D12_SHADER_INPUT_BIND_DESC& resourceBinding, T* reflection);

	private:
		RootConstantBufferData m_rootConstantBuffer;
		std::vector<ResourceVariableData> m_resourceVariables;
		std::vector<ResourceVariableData> m_samplerVariables;

		UInt32 m_rootparameterCount = 0;
	};

	template<typename T>
	inline void HLSLReflection::AddShaderInputBindings(const D3D12_SHADER_INPUT_BIND_DESC& boundResource, T* reflection)
	{
		std::string name(boundResource.Name);

		// Constant Buffer is processed separately. Because it can be either a single buffer or collection of root constants.
		if (boundResource.Type == D3D_SIT_CBUFFER)
		{
			if (m_rootConstantBuffer.name == name)
				return;
			
			auto recourceIt = std::find_if(m_resourceVariables.begin(), m_resourceVariables.end(),
				[&name](const ResourceVariableData& data) { return data.name == name; });

			if (recourceIt != m_resourceVariables.end())
				return; //checking if the resource is already processed because it can exist in multiple shaders

			auto samplerIt = std::find_if(m_samplerVariables.begin(), m_samplerVariables.end(),
				[&name](const ResourceVariableData& data) { return data.name == name; });

			if (samplerIt != m_samplerVariables.end())
				return; //checking if the resource is already processed because it can exist in multiple shaders

			ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByName(boundResource.Name);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			cb->GetDesc(&cbDesc);

			ID3D12ShaderReflectionVariable* var =
				cb->GetVariableByIndex(0);

			D3D12_SHADER_VARIABLE_DESC varDesc = {};
			var->GetDesc(&varDesc);

			ID3D12ShaderReflectionType* type = var->GetType();

			D3D12_SHADER_TYPE_DESC typeDesc = {};
			type->GetDesc(&typeDesc);

			//if constant buffer size per variable is less than Matrix4, then it is considered as a holder of root constants
			if (cbDesc.Size / typeDesc.Members < 32) { //TODO Find better condition for constant buffer checking if possible
				m_rootConstantBuffer.rootParameterIndex = m_rootparameterCount;
				m_rootConstantBuffer.name = name;
				m_rootConstantBuffer.resourceData = boundResource; 
				m_rootConstantBuffer.typeDesc = typeDesc;
				m_rootConstantBuffer.registerData = D3D12_ROOT_CONSTANTS{
					.ShaderRegister = boundResource.BindPoint,
					.RegisterSpace = boundResource.Space,
					.Num32BitValues = cbDesc.Size / 4,
				};

				UInt32 index = 0;
				UInt32 offset = 0;

				auto fillBlock = [this, &offset, &index, type, &typeDesc](bool dynamic) {
					auto& blockItem = this->m_rootConstantBuffer.blocks.emplace_back(RootConstantBlockData{
					.offset = offset,
					.size = 0,
					.isDynamic = dynamic,
						});

					while ((dynamic ? type->GetMemberTypeName(index)[0] == '_' : type->GetMemberTypeName(index)[0] != '_')) {
						ID3D12ShaderReflectionType* memberType = type->GetMemberTypeByIndex(index);

						D3D12_SHADER_TYPE_DESC memberDesc = {};
						memberType->GetDesc(&memberDesc);

						blockItem.variables.push_back(RootConstantVariableData{
							.name = type->GetMemberTypeName(index),
							.index = index,
							.size = (memberDesc.Rows * memberDesc.Columns * 4),
							.offset = offset,
							.variableDesc = memberDesc,
							});
						blockItem.size += (memberDesc.Rows * memberDesc.Columns * 4);

						index++;
						offset += (memberDesc.Rows * memberDesc.Columns * 4);

						if (index >= typeDesc.Members)
							break;
					}
				};


				do {
					auto varName = type->GetMemberTypeName(index);

					if (type->GetMemberTypeName(index)[0] == '_') {
						fillBlock(true);
					}
					else {
						fillBlock(false);
					}
				} while (index < typeDesc.Members);
			}
			else {
				m_resourceVariables.push_back(ResourceVariableData{
					.rootParameterIndex = m_rootparameterCount,
					.name = name,
					.resourceData = boundResource });
			}

			m_rootparameterCount++;
		}
		// Sampler is considered as separate type because it can either contribute as root parameter or as static sampler
		else if (boundResource.Type == D3D_SIT_SAMPLER) {
			m_samplerVariables.push_back(ResourceVariableData{ .rootParameterIndex = 9999, .name = name, .resourceData = boundResource });
		}
		else {
			m_resourceVariables.push_back(ResourceVariableData{ .rootParameterIndex = m_rootparameterCount, .name = name, .resourceData = boundResource });
			m_rootparameterCount++;
		}
	}
}