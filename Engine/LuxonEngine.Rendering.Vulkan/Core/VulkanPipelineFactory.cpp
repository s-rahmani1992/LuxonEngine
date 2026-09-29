#include "vulkan-pch.h"
#include "VulkanPipelineFactory.h"
#include "Mesh/VulkanMeshPipelineModule.h"
#include "Mesh/SPIRVMeshProgram.h"
#include "Rendering/Material.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	VulkanPipelineFactory::VulkanPipelineFactory(VkDevice device)
		:m_device(device)
	{

	}

	ref<MeshShading::VulkanMeshPipelineModule> VulkanPipelineFactory::CreateMeshPipeline(Material* material, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error)
	{
		if (material == nullptr) {
			error = "Material is null";
			return nullptr;
		}

		return CreateMeshPipeline(material->GetProgram().get(), material, renderPass, properties, error);
	}

	ref<MeshShading::VulkanMeshPipelineModule> VulkanPipelineFactory::CreateMeshPipeline(const ShaderProgram* program, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error)
	{
		return CreateMeshPipeline(program, nullptr, renderPass, properties, error);
	}

	ref<MeshShading::VulkanMeshPipelineModule> VulkanPipelineFactory::CreateMeshPipeline(const ShaderProgram* program, Material* material, VkRenderPass renderPass, const MeshShading::MeshPipelineProperties& properties, std::string& error)
	{
		auto meshProgram = dynamic_cast<const MeshShading::SPIRVMeshProgram*>(program);

		if (meshProgram == nullptr) {
			error = "Program is not a mesh shader program";
			return nullptr;
		}

		if (meshProgram->GetPipelineLayout() == VK_NULL_HANDLE) {
			error = "Pipeline layout of the mesh program is not created";
			return nullptr;
		}

		// mesh pipelines have no vertex input and input assembly state, the geometry comes from the mesh stage
		VkPipelineViewportStateCreateInfo viewportStateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = nullptr,
			.scissorCount = 1,
			.pScissors = nullptr,
		};

		//Rasterizer, same as the other rasterization pipelines of the Vulkan backend
		VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampleStateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		//Depth buffer
		VkPipelineDepthStencilStateCreateInfo depthStencilState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = properties.enableDepthTest ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = properties.enableDepthTest ? VK_TRUE : VK_FALSE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		//Blend, one attachment state per render target of the fragment stage
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(meshProgram->GetRenderTargetCount(), colorBlendAttachmentState);

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = (UInt32)colorBlendAttachments.size(),
			.pAttachments = colorBlendAttachments.empty() ? nullptr : colorBlendAttachments.data(),
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
		};

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = (UInt32)dynamicStates.size(),
			.pDynamicStates = dynamicStates.data(),
		};

		auto& stages = meshProgram->GetStageInfos();

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = (UInt32)stages.size(),
			.pStages = stages.data(),
			.pVertexInputState = nullptr,
			.pInputAssemblyState = nullptr,
			.pTessellationState = nullptr,
			.pViewportState = &viewportStateInfo,
			.pRasterizationState = &rasterizationStateInfo,
			.pMultisampleState = &multisampleStateInfo,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendStateInfo,
			.pDynamicState = &dynamicStateInfo,
			.layout = meshProgram->GetPipelineLayout(),
			.renderPass = renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		VkPipeline pipeline = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
			error = "Failed to create mesh pipeline";
			return nullptr;
		}

		return std::make_shared<MeshShading::VulkanMeshPipelineModule>(m_device, pipeline, meshProgram, material);
	}
}
