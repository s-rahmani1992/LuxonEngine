#pragma once
#include "Rendering\ShaderProgram.h"
#include "HLSLShader.h"
#include "HLSLReflection.h"
#include <unordered_map>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12 {
    class RootParameterLayout {
    public:
        RootParameterLayout() = default;

        UInt32 AddParameter(const std::string& name, bool isConstantData = false);

        UInt32 GetRootParameterIndex(const std::string& name) const;

        inline UInt32 GetConstantDataRootIndex() const { return m_constantDataRootIndex; }

        inline UInt32 GetRootParameterCount() const { return m_rootParameterCount; }

    private:
        std::unordered_map<std::string, UInt32> m_rootParameterIndices;
        UInt32 m_constantDataRootIndex = ShaderVariableReflection::InvalidIndex;
        UInt32 m_rootParameterCount = 0;
    };

    struct RenderTargetReflection {
		std::vector<DXGI_FORMAT> formats;
	};

    struct HLSLShaderData {
        D3D12_SHADER_VERSION_TYPE shaderType;
		std::string entryPoint;
		ComPtr<IDxcBlob> byteCode = nullptr;
        ComPtr<ID3D12ShaderReflection> reflection = nullptr;
    };
    
    class HLSLShaderProgram : public ShaderProgram
    {
    public:
        HLSLShaderProgram() = default;
        
        /// <summary>
        /// Gets the parameter layout for this shader program
        /// </summary>
        /// <returns></returns>
        inline HLSLReflection* GetReflectionData() { return &m_reflection; }

        /// <summary>
        /// Gets the root signature for this shader program
        /// </summary>
        /// <returns></returns>
        inline ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_rootSignature; }

        /// <summary>
        /// Gets the root parameter index of every variable of this program. it is created along with the root signature
        /// </summary>
        /// <returns></returns>
        inline const RootParameterLayout& GetRootParameterLayout() const { return m_rootParameterLayout; }

        /// <summary>
        /// Creates Root signature of this program
        /// </summary>
        /// <param name="device"></param>
        /// <returns></returns>
        virtual bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) = 0;

    protected:
        HLSLReflection m_reflection;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        RootParameterLayout m_rootParameterLayout;
    };
}
