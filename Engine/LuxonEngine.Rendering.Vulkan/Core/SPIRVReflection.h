#pragma once
#include "vulkan-pch.h"
#include <Rendering/Material.h>
#include <Rendering/ShaderReflection.h>

namespace LuxonEngine::Rendering::Vulkan {
	struct RenderTargetReflection {
		std::vector<VkFormat> formats;
	};

	struct InputAssemblyReflection {
		std::vector<VkVertexInputAttributeDescription> attributes;
		UInt32 stride = 0;
	};

	class SPIRVVariableReflection {
	public:
		SPIRVVariableReflection() = delete;

		static ShaderStageFlags ToShaderStage(SpvReflectShaderStageFlagBits stage);

		static std::string ShaderStageToString(SpvReflectShaderStageFlagBits stage);

		static ShaderResourceKind ToResourceKind(const SpvReflectDescriptorBinding* descriptor);

		static ShaderScalarType ToScalarType(SpvReflectTypeFlags typeFlags, const SpvReflectNumericTraits& numericTraits);

		static void AddShaderReflection(ShaderVariableReflection& reflection, const SpvReflectShaderModule* shaderReflectionModule);

		static void FillThreadGroupReflection(ShaderThreadGroupReflection& reflection, const SpvReflectShaderModule* shaderReflectionModule);

		static void FillRenderTargetReflection(RenderTargetReflection& reflection, const SpvReflectShaderModule* shaderReflectionModule);

		static void FillInputAssemblyReflection(InputAssemblyReflection& reflection, const SpvReflectShaderModule* shaderReflectionModule);

		static VkDescriptorType ToDescriptorType(ShaderResourceKind kind);

		static bool CreatePipelineLayout(VkDevice device, const ShaderVariableReflection& reflection, UInt32 setCount, VkShaderStageFlags stageFlags,
			VkSampler sampler, std::vector<VkDescriptorSetLayout>& setLayouts, VkPipelineLayout& pipelineLayout, std::string& error);

	private:
		static void AddConstants(ShaderVariableReflection& reflection, const SpvReflectBlockVariable* pushConstant, ShaderStageFlags stage);
	};

	struct DescriptableBufferData {
		std::string name;
		bool isDynamicArray;
		VkDescriptorType descriptorType;
		SpvReflectDescriptorBinding data;
	};

	class SPIRVReflection
	{
	public:
		SPIRVReflection() = default;
		void AddShaderReflection(const SpvReflectShaderModule* shaderReflection);
		UInt32 GetDescriptorLayoutCount();
		void CreatePipelineLayout(const VkDevice device, VkShaderStageFlags stageFlags, const VkSampler sampler, VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout* m_descriptorSetLayout);
		inline std::vector<DescriptableBufferData>& GetDescriptors() { return m_descripters; }
		DescriptableBufferData* GetDescriptorData(const std::string name);
	private:
		std::vector<DescriptableBufferData> m_descripters;
		std::vector<DescriptableBufferData> m_samplers;
	};
}