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
	struct SPIRVShaderData;
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
		SPIRVShaderData CompileShaderStageData(const void* source, size_t size, const DXC::DXCCompileOptions& options, std::string& error);
		VkDevice m_device;

		std::unique_ptr<DXC::DXCCompiler> m_compiler;

		std::map<SpvReflectShaderStageFlagBits, std::wstring> m_validMeshStages = {
			{ SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT, L"ps_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT, L"as_6_6" },
			{ SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT, L"ms_6_6" }
		};
	};
}