#pragma once

#include "vulkan-pch.h"
#include <string>
#include <vector>
#include "Core/SPIRVShaderProgram.h"

namespace LuxonEngine::Rendering::Vulkan::Rasterization {
	class SPIRVRasterizationProgram : public SPIRVShaderProgram
	{
	public:
		static constexpr UInt32 GlobalSetIndex = 0;
		static constexpr UInt32 EntitySetIndex = 1;
		static constexpr VkShaderStageFlags StageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

		SPIRVRasterizationProgram(const std::vector<SPIRVShaderData>& shaders, const VkDevice device);
		virtual ~SPIRVRasterizationProgram() override;

		SPIRVRasterizationProgram(const SPIRVRasterizationProgram&) = delete;
		SPIRVRasterizationProgram& operator=(const SPIRVRasterizationProgram&) = delete;

		virtual ShaderProgramType GetType() override { return ShaderProgramType::Rasterization; }

		bool InitializePipelineLayout(std::string& error);

		inline const std::vector<VkPipelineShaderStageCreateInfo>& GetStageInfos() const { return m_stageInfos; }
		inline VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		inline const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }

	private:
		VkShaderModule CreateStage(const SPIRVShaderData& shader, std::string& entryPoint);

		static bool RemapDescriptorSets(SpvReflectShaderModule& reflectionModule);

		VkShaderModule m_vertexShader = VK_NULL_HANDLE;
		VkShaderModule m_geometryShader = VK_NULL_HANDLE;
		VkShaderModule m_pixelShader = VK_NULL_HANDLE;

		std::string m_vertexEntryPoint;
		std::string m_geometryEntryPoint;
		std::string m_pixelEntryPoint;

		std::vector<VkPipelineShaderStageCreateInfo> m_stageInfos;

		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	};
}
