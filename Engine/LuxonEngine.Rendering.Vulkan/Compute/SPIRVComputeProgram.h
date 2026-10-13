#pragma once
#include "vulkan-pch.h"
#include <string>
#include <vector>
#include "Core/SPIRVShaderProgram.h"

namespace LuxonEngine::Rendering::Vulkan::Compute {
	class SPIRVComputeProgram : public SPIRVShaderProgram {
	public:
		static constexpr VkShaderStageFlags StageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		SPIRVComputeProgram(const SPIRVShaderData& shader, const VkDevice device);
		virtual ~SPIRVComputeProgram() override;

		SPIRVComputeProgram(const SPIRVComputeProgram&) = delete;
		SPIRVComputeProgram& operator=(const SPIRVComputeProgram&) = delete;

		virtual ShaderProgramType GetType() override { return ShaderProgramType::Compute; }

		bool InitializePipelineLayout(std::string& error);

		inline const VkPipelineShaderStageCreateInfo& GetStageInfo() const { return m_stageInfo; }
		inline VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		inline const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }
		inline const ShaderThreadGroupReflection& GetThreadGroupReflection() const { return m_threadGroupReflection; }

	private:
		VkShaderModule m_computeShader = VK_NULL_HANDLE;
		std::string m_entryPoint;
		VkPipelineShaderStageCreateInfo m_stageInfo{};

		ShaderThreadGroupReflection m_threadGroupReflection{};

		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	};
}
