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
		virtual ~SPIRVShaderProgram() = default;
		inline SPIRVReflection& GetReflection() { return m_reflection; }
	protected:
		void InitializeSampler();
		VkDevice m_device;
		VkSampler m_sampler;
		SPIRVReflection m_reflection;
	};
}