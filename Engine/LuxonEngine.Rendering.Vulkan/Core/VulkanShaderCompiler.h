#pragma once
#include "vulkan-pch.h"
#include "Rendering/ShaderCompiler.h"
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

	class VulkanShaderCompiler : public ShaderCompiler
	{
	public:

		VulkanShaderCompiler(VkDevice device);
		~VulkanShaderCompiler();

		VulkanShaderCompiler(const VulkanShaderCompiler&) = delete;
		VulkanShaderCompiler& operator=(const VulkanShaderCompiler&) = delete;

		bool Initialize();

		virtual ShaderProgram* CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& properties, std::string& error) override;
	private:
		DXC::DXCCompileOptions CreateCompileOptions(const std::wstring& includeDir) const;
		ref<SPIRVShader> CompileShaderStage(const void* source, size_t size, const DXC::DXCCompileOptions& options, Vulkan_Shader_Type shaderType, std::string& error);
		VkDevice m_device;

		std::unique_ptr<DXC::DXCCompiler> m_compiler;
	};
}