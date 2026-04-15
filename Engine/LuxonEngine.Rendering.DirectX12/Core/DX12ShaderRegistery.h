#pragma once
#include "pch.h"
#include "Rendering/ShaderRegistery.h"
#include <comdef.h>
#include <boost/uuid/uuid.hpp>
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	class HLSLShaderProgram;
	class HLSLShader;
	enum DX12_Shader_Type;

	class DX12ShaderRegistery : public ShaderRegistery
	{
	public:

		DX12ShaderRegistery();
		~DX12ShaderRegistery();
		void Initialize(const ComPtr<ID3D12Device10>& device);
		ref<HLSLShaderProgram> GetShaderProgram(const std::string& name);
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) override;
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) override;
	
	private:

		ref<HLSLShader> CompileShaderStage(const DxcBuffer* sourceBuffer, DX12_Shader_Type shaderType, std::string& error);
		bool CompileInternal(const DxcBuffer* sourceBuffer, ComPtr<IDxcBlob>& pshaderData, ComPtr<IUnknown>& reflection, UInt32 argumentCount, std::string& error);
	
	private:
		ComPtr<ID3D12Device10> m_device;
		std::map<std::string, ref<HLSLShaderProgram>> m_specialShaders;
		std::map<boost::uuids::uuid, ref<HLSLShaderProgram>> m_shaders;

		IDxcUtils* m_utils;
		IDxcIncludeHandler* m_includeHandler;
		IDxcCompiler3* m_dxcCompiler;
		std::vector<LPWSTR> m_compileArguments;
		UInt32 m_minArguments;
		const UInt32 m_mainIndex = 1;
		const UInt32 m_targetIndex = 3;
	};
}


