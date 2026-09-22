#pragma once
#include "pch.h"
#include "Rendering/ShaderRegistery.h"
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
	class HLSLShader;
	enum DX12_Shader_Type;

	class DX12ShaderRegistery : public ShaderRegistery
	{
	public:
		DX12ShaderRegistery();
		~DX12ShaderRegistery();

		DX12ShaderRegistery(const DX12ShaderRegistery&) = delete;
		DX12ShaderRegistery& operator=(const DX12ShaderRegistery&) = delete;

		bool Initialize(const ComPtr<ID3D12Device10>& device);
		ref<HLSLShaderProgram> GetShaderProgram(const std::string& name);

		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) override;
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) override;
		virtual ShaderProgram* CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& properties, std::string& error) override;
		virtual ref<ShaderProgram> GetProgramByGUID(boost::uuids::uuid guid) override;
	private:

		DXC::DXCCompileOptions CreateCompileOptions(const std::wstring& includeDir) const;
		ref<HLSLShader> CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, DX12_Shader_Type shaderType, std::string& error);
	
	private:
		ComPtr<ID3D12Device10> m_device;
		std::map<std::string, ref<HLSLShaderProgram>> m_specialShaders;
		std::map<boost::uuids::uuid, ref<HLSLShaderProgram>> m_shaders;

		std::unique_ptr<DXC::DXCCompiler> m_compiler;
	};
}


