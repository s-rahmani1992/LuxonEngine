#pragma once
#include "pch.h"
#include "Rendering/ShaderCompiler.h"
#include <comdef.h>
#include <boost/uuid/uuid.hpp>
#include <map>
#include <memory>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DXC {
	class DXCCompiler;
	struct DXCCompileOptions;
}

namespace LuxonEngine::Rendering::DX12 {
	class HLSLShaderProgram;
	class HLSLShaderData;
	class HLSLShader;
	enum DX12_Shader_Type;

	class DX12ShaderCompiler : public ShaderCompiler
	{
	public:
		DX12ShaderCompiler();
		~DX12ShaderCompiler();

		DX12ShaderCompiler(const DX12ShaderCompiler&) = delete;
		DX12ShaderCompiler& operator=(const DX12ShaderCompiler&) = delete;

		bool Initialize(const ComPtr<ID3D12Device10>& device);

		virtual ShaderProgram* CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& properties, std::string& error) override;
	private:

		DXC::DXCCompileOptions CreateCompileOptions(const std::wstring& includeDir) const;

		bool DiscoverStages(const void* source, size_t size, const DXC::DXCCompileOptions& baseOptions,
			const std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& validStages,
			std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& outStages, std::string& error);
		ref<HLSLShader> CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, DX12_Shader_Type shaderType, std::string& error);
		HLSLShaderData CompileShaderStageWithReflection(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error);
	private:
		ComPtr<ID3D12Device10> m_device;

		std::unique_ptr<DXC::DXCCompiler> m_compiler;

		const std::map<D3D12_SHADER_VERSION_TYPE, std::wstring> m_validRasterStages = {
			{ D3D12_SHVER_VERTEX_SHADER, L"vs_6_6" },
			{ D3D12_SHVER_GEOMETRY_SHADER, L"gs_6_6" },
			{ D3D12_SHVER_PIXEL_SHADER, L"ps_6_6" },
		};

		std::map<D3D12_SHADER_VERSION_TYPE, std::wstring> m_validMeshStages = {
			{ D3D12_SHVER_PIXEL_SHADER, L"ps_6_6" },
			{ D3D12_SHVER_AMPLIFICATION_SHADER, L"as_6_6" },
			{ D3D12_SHVER_MESH_SHADER, L"ms_6_6" }
		};
	};
}


