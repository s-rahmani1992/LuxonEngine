#pragma once
#include "pch.h"
#include <string>
#include "HLSLShaderProgram.h"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	class HLSLShader;
}

namespace QuantumEngine::Rendering::DX12::Rasterization {
	class HLSLRasterizationProgram : public HLSLShaderProgram {
	public:
		HLSLRasterizationProgram(const std::vector<ref<HLSLShader>>& shaders);
		virtual ~HLSLRasterizationProgram() = default;

		/// <summary>
		/// Creates the DirectX 12 root signature for this shader program
		/// </summary>
		/// <param name="device"></param>
		/// <returns></returns>
		bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device, std::string& error) override;

		/// <summary>
		/// Gets the vertex shader if exists
		/// </summary>
		/// <returns></returns>
		inline ref<HLSLShader> GetVertexShader() const { return m_vertexShader; }

		/// <summary>
		/// Gets the pixel shader if exists
		/// </summary>
		/// <returns></returns>
		inline ref<HLSLShader> GetPixelShader() const { return m_pixelShader; }

		/// <summary>
		/// Gets the geometry shader if exists
		/// </summary>
		/// <returns></returns>
		inline ref<HLSLShader> GetGeometryShader() const { return m_geometryShader; }

	private:
		ref<HLSLShader> m_vertexShader;
		ref<HLSLShader> m_pixelShader;
		ref<HLSLShader> m_geometryShader;
	};
}