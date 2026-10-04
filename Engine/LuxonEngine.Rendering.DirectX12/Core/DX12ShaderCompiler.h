#pragma once
#include "pch.h"
#include "Rendering/ShaderCompiler.h"
#include <comdef.h>
#include <boost/uuid/uuid.hpp>
#include <map>
#include <memory>
#include <vector>
#include <string>

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
		virtual void AddIncludePath(const std::wstring& includePath) override;
	private:

		DXC::DXCCompileOptions CreateCompileOptions(const std::wstring& currentFileDir) const;

		bool DiscoverStages(const void* source, size_t size, 
			const DXC::DXCCompileOptions& baseOptions, ShaderProgramType shaderType,
			std::map<D3D12_SHADER_VERSION_TYPE, std::wstring>& outStages, std::string& error);
		HLSLShaderData CompileShaderStageWithReflection(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error);
	private:
		ComPtr<ID3D12Device10> m_device;

		std::unique_ptr<DXC::DXCCompiler> m_compiler;
		std::vector<std::wstring> m_includePaths;
	};
}


