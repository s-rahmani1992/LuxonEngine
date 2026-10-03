#pragma once
#include "pch.h"
#include <string>
#include "HLSLShaderProgram.h"

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12 {
	class HLSLShader;
}

namespace LuxonEngine::Rendering::DX12::Rasterization {
	class HLSLRasterizationProgram : public HLSLShaderProgram {
	public:
		HLSLRasterizationProgram(const std::vector<HLSLShaderData>& shaders);
		virtual ~HLSLRasterizationProgram() = default;

		virtual ShaderProgramType GetType() override { return ShaderProgramType::Rasterization; }

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
		inline IDxcBlob* GetVertexShader() const { return m_vertexShader.Get(); }

		/// <summary>
		/// Gets the pixel shader if exists
		/// </summary>
		/// <returns></returns>
		inline IDxcBlob* GetPixelShader() const { return m_pixelShader.Get(); }

		/// <summary>
		/// Gets the geometry shader if exists
		/// </summary>
		/// <returns></returns>
		inline IDxcBlob* GetGeometryShader() const { return m_geometryShader.Get(); }

	private:
		ComPtr<IDxcBlob> m_vertexShader = nullptr;
		ComPtr<IDxcBlob> m_geometryShader = nullptr;
		ComPtr<IDxcBlob> m_pixelShader = nullptr;
	};
}