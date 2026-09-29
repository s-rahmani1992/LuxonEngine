#pragma once
#include "vulkan-pch.h"
#include <string>
#include <vector>
#include "Core/SPIRVShaderProgram.h"

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	class SPIRVMeshProgram : public SPIRVShaderProgram {
	public:
		// descriptor set of the material and global variables, bound once per pipeline
		static constexpr UInt32 GlobalSetIndex = 0;
		// descriptor set of the per entity internal variables, bound for every entity
		static constexpr UInt32 EntitySetIndex = 1;

		SPIRVMeshProgram(const std::vector<SPIRVShaderData>& shaders, const VkDevice device);
		virtual ~SPIRVMeshProgram() override;

		SPIRVMeshProgram(const SPIRVMeshProgram&) = delete;
		SPIRVMeshProgram& operator=(const SPIRVMeshProgram&) = delete;

		virtual ShaderProgramType GetType() override { return ShaderProgramType::Mesh; }

		inline const std::vector<VkPipelineShaderStageCreateInfo>& GetStageInfos() const { return m_stageInfos; }
		inline const ShaderThreadGroupReflection& GetThreadGroupReflection() const { return m_threadGroupReflection; }

		inline VkShaderModule GetTaskShader() const { return m_taskShader; }
		inline VkShaderModule GetMeshShader() const { return m_meshShader; }
		inline VkShaderModule GetPixelShader() const { return m_pixelShader; }

	private:
		VkShaderModule CreateStage(const SPIRVShaderData& shader, std::string& entryPoint);

		static bool RemapDescriptorSets(SpvReflectShaderModule& reflectionModule);

		VkShaderModule m_taskShader = VK_NULL_HANDLE;
		VkShaderModule m_meshShader = VK_NULL_HANDLE;
		VkShaderModule m_pixelShader = VK_NULL_HANDLE;

		// the stage infos point to these names
		std::string m_taskEntryPoint;
		std::string m_meshEntryPoint;
		std::string m_pixelEntryPoint;

		std::vector<VkPipelineShaderStageCreateInfo> m_stageInfos;
		ShaderThreadGroupReflection m_threadGroupReflection{};
	};
}
