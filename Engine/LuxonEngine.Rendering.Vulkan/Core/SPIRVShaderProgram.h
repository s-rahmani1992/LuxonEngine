#pragma once
#include "vulkan-pch.h"
#include "Rendering/ShaderProgram.h"
#include "Core/SPIRVReflection.h"
#include <dxcapi.h>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::Vulkan {
	struct SPIRVShaderData {
		VkShaderStageFlagBits shaderType;
		std::string entryPoint;
		ComPtr<IDxcBlob> byteCode = nullptr;
	};


	class SPIRVShaderProgram : public ShaderProgram {
	public:
		static constexpr UInt32 GlobalSetIndex = 0;
		static constexpr UInt32 EntitySetIndex = 1;

		virtual ~SPIRVShaderProgram() = default;
		inline SPIRVReflection& GetReflection() { return m_reflection; }
	protected:
		void InitializeSampler();
		static bool RemapDescriptorSets(SpvReflectShaderModule& reflectionModule);
		VkDevice m_device;
		VkSampler m_sampler;
		SPIRVReflection m_reflection;
	};
}