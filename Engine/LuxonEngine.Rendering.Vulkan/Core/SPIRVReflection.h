#pragma once
#include "vulkan-pch.h"
#include <Rendering/Material.h>

namespace QuantumEngine::Rendering::Vulkan {
	struct PushConstantVariableData {
		std::string name;
		SpvReflectBlockVariable variableDesc;
	};

	struct PushConstantBlockData {
		UInt32 offset;
		UInt32 size;
		bool isDynamic;
		std::vector<PushConstantVariableData> variables;
	};

	struct PushConstantBufferData { //TODO Add shader stage flags to make it more optimal for binding
		std::string name;
		SpvReflectBlockVariable data;
		std::vector<PushConstantBlockData> blocks;
	};

	struct DescriptableBufferData {
		std::string name;
		bool isDynamicArray;
		UInt32 offsetIndex;
		VkDescriptorType descriptorType;
		SpvReflectDescriptorBinding data;
	};

	class SPIRVReflection
	{
	public:
		SPIRVReflection() = default;
		void AddShaderReflection(const SpvReflectShaderModule* shaderReflection, bool isRayTracing = false);
		UInt32 GetDescriptorLayoutCount();
		UInt32 GetDynamicDescriptorCount();
		void CreatePipelineLayout(const VkDevice device, VkShaderStageFlags stageFlags, const VkSampler sampler, VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout* m_descriptorSetLayout);
		MaterialReflection CreateMaterialReflection();
		inline PushConstantBufferData& GetPushConstants() { return m_pushConstant; }
		PushConstantBlockData* GetPushConstantBlockData(const std::string& name);
		inline std::vector<DescriptableBufferData>& GetDescriptors() { return m_descripters; }
		DescriptableBufferData* GetDescriptorData(const std::string name);
		void Initializes();
	private:
		PushConstantBufferData m_pushConstant;
		std::vector<DescriptableBufferData> m_descripters;
		std::vector<DescriptableBufferData> m_samplers;
	};
}