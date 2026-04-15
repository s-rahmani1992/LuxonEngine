#include "vulkan-pch.h"
#include "VulkanRasterizationPipelineModule.h"
#include "Core/Mesh.h"
#include "../Core/VulkanMeshController.h"
#include "Core/GameEntity.h"
#include "SPIRVRasterizationProgram.h"
#include "Rendering/Renderer.h"
#include "Rendering/Material.h"
#include "VulkanRasterizationMaterial.h"
#include "SPIRVRasterizationProgram.h"

VkVertexInputBindingDescription QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::s_bindingDescriptions = {
	.binding = 0,
	.stride = sizeof(Vertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

VkVertexInputAttributeDescription QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::s_attributeDescriptions[3] = {
	{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position) },
	{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
	{ .location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
};

VkPipelineVertexInputStateCreateInfo QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::s_vertexInputInfo = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	.pNext = nullptr,
	.flags = 0,
	.vertexBindingDescriptionCount = 1,
	.pVertexBindingDescriptions = &s_bindingDescriptions,
	.vertexAttributeDescriptionCount = 3,
	.pVertexAttributeDescriptions = s_attributeDescriptions,
};

QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::VulkanRasterizationPipelineModule(const VkDevice device)
	:m_device(device)
{
}

QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::~VulkanRasterizationPipelineModule()
{
	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::RenderCommand(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	VkDeviceSize offsets[] = { 0 };
	auto indexBuffer = m_meshController->GetIndexBuffer();
	auto vertexBuffer = m_meshController->GetVertexBuffer();
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	m_material->BindValues(commandBuffer);
	m_material->BindDynamicValues(commandBuffer, m_offset.data(), (UInt32)m_offset.size());
	vkCmdDrawIndexed(commandBuffer, m_mesh->GetIndexCount(), 1, 0, 0, 0);
}

bool QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::Initialize(const ref<GameEntity>& entity, ref<VulkanRasterizationMaterial> material, const VkRenderPass renderPass)
{
	m_program = std::dynamic_pointer_cast<SPIRVRasterizationProgram>(entity->GetRenderer()->GetMaterial()->GetProgram());
	m_offset = std::vector<UInt32>(m_program->GetReflection().GetDynamicDescriptorCount(), 0);
	m_mesh = entity->GetRenderer()->GetMesh();
	m_meshController = std::dynamic_pointer_cast<VulkanMeshController>(m_mesh->GetGPUHandle());
	m_material = material;
	
	VkPipelineInputAssemblyStateCreateInfo pInputAssemblyInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = nullptr,
		.scissorCount = 1,
		.pScissors = nullptr,
	};

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

	VkPipelineDepthStencilStateCreateInfo depthStencilState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = -1.0f,
		.maxDepthBounds = 1.0f,
	};

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

	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
	};

	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (UInt32)dynamicStates.size(),
		.pDynamicStates = dynamicStates.data(),
	};

	auto& stages = m_program->GetStageInfos();
	
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = (UInt32)stages.size(),
		.pStages = stages.data(),
		.pVertexInputState = &s_vertexInputInfo,
		.pInputAssemblyState = &pInputAssemblyInfo,
		.pTessellationState = nullptr,
		.pViewportState = &viewportStateInfo,
		.pRasterizationState = &rasterizationStateInfo,
		.pMultisampleState = &multisampleStateInfo,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendStateInfo,
		.pDynamicState = &dynamicStateInfo,
		.layout = m_program->GetPipelineLayout(),
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,

	};

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
		return false;
	}

	return true;
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationPipelineModule::SetDescriptorOffset(const std::string& name, UInt32 offset)
{
	auto descriptorData = m_program->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	m_offset[descriptorData->offsetIndex] = offset;
}
