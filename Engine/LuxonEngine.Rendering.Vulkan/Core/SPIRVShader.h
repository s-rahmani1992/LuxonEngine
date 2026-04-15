#pragma once
#include "vulkan-pch.h"
#include "Rendering/Shader.h"

namespace QuantumEngine::Rendering::Vulkan{
	enum Vulkan_Shader_Type {
		Vulkan_Vertex = 0,
		Vulkan_Fragment = 1,
		Vulkan_Geometry = 2,
	};

	class SPIRVShader : public Shader
	{
	public:
		SPIRVShader(Byte* byteCode, UInt64 codeSize, Vulkan_Shader_Type shaderType, VkDevice device, const std::string& main);
		~SPIRVShader() override;

		/// <summary>
		/// Gets type of the shader, ex. vertex, fragment, etc.
		/// </summary>
		/// <returns></returns>
		inline Vulkan_Shader_Type GetShaderType() const { return m_shaderType; }

		/// <summary>
		/// Gets the name of shader entry name, ex. "main"
		/// </summary>
		/// <returns></returns>
		inline std::string& GetEntryPoint() { return m_mainEntry; }

		/// <summary>
		/// Gets the Vulkan shader module
		/// </summary>
		/// <returns></returns>
		inline VkShaderModule GetShaderModule() const { return m_module; }

		inline SpvReflectShaderModule* GetReflectionModule() { return &m_reflectionModule; }

	private:
		Vulkan_Shader_Type m_shaderType;
		VkDevice m_device;

		VkShaderModule m_module;
		SpvReflectShaderModule m_reflectionModule;
		std::string m_mainEntry;
	};
}