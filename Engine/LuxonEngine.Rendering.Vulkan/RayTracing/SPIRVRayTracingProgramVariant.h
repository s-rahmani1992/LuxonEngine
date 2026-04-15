#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class SPIRVRayTracingProgramVariant {
	public:
		SPIRVRayTracingProgramVariant(Byte* byteCode, UInt32 codesize);
		~SPIRVRayTracingProgramVariant();
		std::vector<VkPipelineShaderStageCreateInfo> GetStages();
		SpvReflectShaderModule* GetReflectionModule() { return &m_reflectionModule; }
		void GetBindingAndSet(const std::string& name, UInt32* binding, UInt32* set);
	private:
		Byte* m_byteCode;
		UInt32 m_codeSize;

		VkDevice m_device;
		VkShaderModule m_rtModule;
		SpvReflectShaderModule m_reflectionModule;
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
	};
}