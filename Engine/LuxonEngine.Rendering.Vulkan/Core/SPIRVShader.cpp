#include "vulkan-pch.h"

#include "SPIRVShader.h"
#include "spirv_reflect.h"

QuantumEngine::Rendering::Vulkan::SPIRVShader::SPIRVShader(Byte* byteCode, UInt64 codeSize, Vulkan_Shader_Type shaderType, VkDevice device, const std::string& main)
	:Shader(byteCode, codeSize, (Int32)shaderType), m_device(device), m_shaderType(shaderType), m_mainEntry(main)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = codeSize,
		.pCode = reinterpret_cast<UInt32*>(m_byteCode),
	};

	vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &m_module);

	spvReflectCreateShaderModule(m_codeSize, m_byteCode, &m_reflectionModule);
}

QuantumEngine::Rendering::Vulkan::SPIRVShader::~SPIRVShader()
{
	vkDestroyShaderModule(m_device, m_module, nullptr);
}
