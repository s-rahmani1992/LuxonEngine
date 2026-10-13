#include "vulkan-pch.h"
#include "SPIRVShaderProgram.h"
#include "Rendering/ShaderInternalNames.h"

void LuxonEngine::Rendering::Vulkan::SPIRVShaderProgram::InitializeSampler()
{
	VkSamplerCreateInfo samplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 0,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	auto result = vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &m_sampler);
}

bool LuxonEngine::Rendering::Vulkan::SPIRVShaderProgram::RemapDescriptorSets(SpvReflectShaderModule& reflectionModule)
{
	for (UInt32 i = 0; i < reflectionModule.descriptor_binding_count; i++) {
		auto& descriptor = reflectionModule.descriptor_bindings[i];

		bool isPerEntity = descriptor.name != nullptr && ShaderInternalNames::IsPerEntity(descriptor.name);
		UInt32 targetSet = isPerEntity ? EntitySetIndex : GlobalSetIndex;

		if (descriptor.set == targetSet)
			continue;

		if (spvReflectChangeDescriptorBindingNumbers(&reflectionModule, &descriptor,
			(UInt32)SPV_REFLECT_BINDING_NUMBER_DONT_CHANGE, targetSet) != SPV_REFLECT_RESULT_SUCCESS)
			return false;
	}

	return true;
}
