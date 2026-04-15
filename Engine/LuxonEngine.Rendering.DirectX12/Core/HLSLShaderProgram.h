#pragma once
#include "Rendering\ShaderProgram.h"
#include "HLSLShader.h"
#include "HLSLReflection.h"
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
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
        /// Creates Root signature of this program
        /// </summary>
        /// <param name="device"></param>
        /// <returns></returns>
        virtual bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) = 0;

    protected:
        HLSLReflection m_reflection;
        ComPtr<ID3D12RootSignature> m_rootSignature;
    };
}
