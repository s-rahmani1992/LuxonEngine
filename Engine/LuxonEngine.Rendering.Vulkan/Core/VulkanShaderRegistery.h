#pragma once
#include "vulkan-pch.h"
#include "Rendering/ShaderRegistery.h"
#include <vector>
#include <map>
#include <memory>
#include <boost/uuid/uuid.hpp>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DXC {
	class DXCCompiler;
	struct DXCCompileOptions;
}

namespace LuxonEngine::Rendering::Vulkan {
	class SPIRVShader;
	class SPIRVShaderProgram;
	enum Vulkan_Shader_Type;

	class VulkanShaderRegistery : public ShaderRegistery
	{
	public:

		VulkanShaderRegistery(VkDevice device);
		~VulkanShaderRegistery();

		VulkanShaderRegistery(const VulkanShaderRegistery&) = delete;
		VulkanShaderRegistery& operator=(const VulkanShaderRegistery&) = delete;

		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) override;
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) override;
		virtual ShaderProgram* CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& properties, std::string& error) override;
		virtual ref<ShaderProgram> GetProgramByGUID(boost::uuids::uuid guid) override;
		ref<SPIRVShaderProgram> GetShaderPrograms(const std::string& name);
		bool Initialize();
	private:
		DXC::DXCCompileOptions CreateCompileOptions(const std::wstring& includeDir) const;
		ref<SPIRVShader> CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, Vulkan_Shader_Type shaderType, std::string& error);
		std::map<boost::uuids::uuid, ref<SPIRVShaderProgram>> m_registeredPrograms;
		std::map<std::string, ref<SPIRVShaderProgram>> m_specialPrograms;
		VkDevice m_device;

		std::unique_ptr<DXC::DXCCompiler> m_compiler;
	};
}