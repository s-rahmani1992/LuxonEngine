#pragma once
#include "vulkan-pch.h"
#include "Rendering/ShaderRegistery.h"
#include <vector>
#include <map>
#include <boost/uuid/uuid.hpp>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::Vulkan {
	class SPIRVShader;
	class SPIRVShaderProgram;
	enum Vulkan_Shader_Type;

	class VulkanShaderRegistery : public ShaderRegistery
	{
	public:

		VulkanShaderRegistery(VkDevice device);
		~VulkanShaderRegistery();
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) override;
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) override;
		ref<SPIRVShaderProgram> GetShaderPrograms(const std::string& name);
		void Initialize();
	private:
		ref<SPIRVShader> CompileShaderStage(const DxcBuffer* sourceBuffer, Vulkan_Shader_Type shaderType, const std::string entryName, std::string& error);
		std::map<boost::uuids::uuid, ref<SPIRVShaderProgram>> m_registeredPrograms;
		std::map<std::string, ref<SPIRVShaderProgram>> m_specialPrograms;
		VkDevice m_device;

		IDxcUtils* m_utils;
		IDxcIncludeHandler* m_includeHandler;
		IDxcCompiler3* m_dxcCompiler;

		std::vector<LPWSTR> m_compileArguments;
		const UInt32 m_mainIndex = 1;
		const UInt32 m_targetIndex = 3;
		const UInt32 m_minArguments = 12;
	};
}