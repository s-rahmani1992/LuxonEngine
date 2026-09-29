#include "vulkan-pch.h"
#include "SPIRVMeshProgram.h"
#include "Rendering/ShaderInternalNames.h"

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	SPIRVMeshProgram::SPIRVMeshProgram(const std::vector<SPIRVShaderData>& shaders, const VkDevice device)
	{
		m_device = device;
		m_sampler = VK_NULL_HANDLE;

		for (auto& shader : shaders) {
			if (shader.byteCode == nullptr)
				continue;

			if (shader.shaderType == VK_SHADER_STAGE_TASK_BIT_EXT) {
				m_taskShader = CreateStage(shader, m_taskEntryPoint);
			}
			else if (shader.shaderType == VK_SHADER_STAGE_MESH_BIT_EXT) {
				m_meshShader = CreateStage(shader, m_meshEntryPoint);
			}
			else if (shader.shaderType == VK_SHADER_STAGE_FRAGMENT_BIT) {
				m_pixelShader = CreateStage(shader, m_pixelEntryPoint);
			}
		}

		// the entry point strings are not moved after this point, so the stage infos can point to them
		auto addStageInfo = [this](VkShaderStageFlagBits stage, VkShaderModule module, const std::string& entryPoint) {
			if (module == VK_NULL_HANDLE)
				return;

			m_stageInfos.push_back(VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = stage,
				.module = module,
				.pName = entryPoint.c_str(),
				});
		};

		addStageInfo(VK_SHADER_STAGE_TASK_BIT_EXT, m_taskShader, m_taskEntryPoint);
		addStageInfo(VK_SHADER_STAGE_MESH_BIT_EXT, m_meshShader, m_meshEntryPoint);
		addStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, m_pixelShader, m_pixelEntryPoint);
	}

	SPIRVMeshProgram::~SPIRVMeshProgram()
	{
		if (m_taskShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_taskShader, nullptr);

		if (m_meshShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_meshShader, nullptr);

		if (m_pixelShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(m_device, m_pixelShader, nullptr);
	}

	VkShaderModule SPIRVMeshProgram::CreateStage(const SPIRVShaderData& shader, std::string& entryPoint)
	{
		entryPoint = shader.entryPoint;

		// the reflection module keeps its own copy of the code, the descriptor sets are remapped in that copy
		SpvReflectShaderModule reflectionModule;

		if (spvReflectCreateShaderModule(shader.byteCode->GetBufferSize(), shader.byteCode->GetBufferPointer(), &reflectionModule) != SPV_REFLECT_RESULT_SUCCESS)
			return VK_NULL_HANDLE;

		if (RemapDescriptorSets(reflectionModule) == false) {
			spvReflectDestroyShaderModule(&reflectionModule);
			return VK_NULL_HANDLE;
		}

		VkShaderModuleCreateInfo shaderModuleCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = spvReflectGetCodeSize(&reflectionModule),
			.pCode = spvReflectGetCode(&reflectionModule),
		};

		VkShaderModule module = VK_NULL_HANDLE;

		if (vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &module) == VK_SUCCESS) {
			if (shader.shaderType == VK_SHADER_STAGE_TASK_BIT_EXT)
				SPIRVVariableReflection::FillThreadGroupReflection(m_threadGroupReflection, &reflectionModule);

			SPIRVVariableReflection::AddShaderReflection(m_variableReflection, &reflectionModule);
		}

		spvReflectDestroyShaderModule(&reflectionModule);
		return module;
	}

	bool SPIRVMeshProgram::RemapDescriptorSets(SpvReflectShaderModule& reflectionModule)
	{
		// only the set changes. the binding numbers come from the declaration order of the whole file,
		// so they are unique in both sets and the same in every stage
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
}
