#include "vulkan-pch.h"
#include "VulkanSplinePipelineModule.h"
#include "Rendering/SplineRenderer.h"
#include "Core/VulkanUtilities.h"
#include "Compute/SPIRVComputeProgram.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Rasterization/VulkanRasterizationMaterial.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanBufferFactory.h"

VkVertexInputBindingDescription QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::s_bindingDescriptions = {
	.binding = 0,
	.stride = sizeof(SplineVertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

VkVertexInputAttributeDescription QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::s_attributeDescriptions[4] = {
	{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SplineVertex, position) },
	{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(SplineVertex, uv) },
	{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SplineVertex, normal) },
	{.location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SplineVertex, tangent) },
};

VkPipelineVertexInputStateCreateInfo QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::s_vertexInputInfo = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	.pNext = nullptr,
	.flags = 0,
	.vertexBindingDescriptionCount = 1,
	.pVertexBindingDescriptions = &s_bindingDescriptions,
	.vertexAttributeDescriptionCount = 4,
	.pVertexAttributeDescriptions = s_attributeDescriptions,
};

QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::VulkanSplinePipelineModule(const VkDevice device)
	:m_device(device)
{
}

QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::~VulkanSplinePipelineModule()
{
	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);

	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::Initialize(const SplineEntityData& splineEntity, const VkRenderPass renderPass, const VkDescriptorPool pool)
{
	m_splineRenderer = splineEntity.splineRenderer;
	m_computeProgram = splineEntity.computeProgram;

	if(InitializeVertexBuffer(splineEntity) == false)
		return false;

	if(InitializeComputePipeline(splineEntity, pool) == false)
		return false;

	if(InitializeGraphicsPipeline(splineEntity, renderPass) == false)
		return false;

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::ComputeCommand(VkCommandBuffer commandBuffer)
{
	if (m_splineRenderer->IsDirty()) {
		m_splineParameters.startPoint = m_splineRenderer->GetCurve().m_point1;
		m_splineParameters.midPoint = m_splineRenderer->GetCurve().m_point2;
		m_splineParameters.endPoint = m_splineRenderer->GetCurve().m_point3;
		m_splineParameters.width = m_splineRenderer->GetWidth();
		m_splineParameters.length = m_splineRenderer->GetCurve().InterpolateLength(1.0f);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
		vkCmdPushConstants(commandBuffer, m_computeProgram->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SplineParameters), &m_splineParameters);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computeProgram->GetPipelineLayout(), 0, 1, &m_computeDescriptorSet, 0, 0);

		vkCmdDispatch(commandBuffer, m_splineRenderer->GetSegments() + 1, 1, 1);
	}
}

void QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::RenderCommand(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, offsets);
	vkCmdPushConstants(commandBuffer, m_program->GetPipelineLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, m_widthOffset, sizeof(Float), &m_splineParameters.width);

	m_material->BindValues(commandBuffer);
	m_material->BindDynamicValues(commandBuffer, m_offset.data(), (UInt32)m_offset.size());
	vkCmdDraw(commandBuffer, m_splineRenderer->GetSegments() + 1, 1, 0, 0);

}

void QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::WriteOffset(const std::string name, UInt32 offset)
{
	auto descriptorData = m_program->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	m_offset[descriptorData->offsetIndex] = offset;
}

bool QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::InitializeVertexBuffer(const SplineEntityData& splineEntity)
{
	auto bufferFactory = VulkanDeviceManager::Instance()->GetBufferFactory();

	if(bufferFactory->CreateBuffer(sizeof(SplineVertex) * (m_splineRenderer->GetSegments() + 1), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vertexBuffer, &m_vertexBufferMemory) == false)
		return false;

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::InitializeComputePipeline(const SplineEntityData& splineEntity, const VkDescriptorPool pool)
{
	VkComputePipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = splineEntity.computeProgram->GetComputeStageInfo(),
		.layout = splineEntity.computeProgram->GetPipelineLayout(),
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS)
		return false;

	auto& layouts = splineEntity.computeProgram->GetDiscriptorLayouts();

	VkDescriptorSetAllocateInfo descSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = layouts.data(),
	};

	if (vkAllocateDescriptorSets(m_device, &descSetAlloc, &m_computeDescriptorSet) != VK_SUCCESS)
		return false;

	auto descriptorData = splineEntity.computeProgram->GetReflection().GetDescriptorData("_vertexBuffer");

	VkDescriptorBufferInfo descBufferInfo{
		.buffer = m_vertexBuffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};

	VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_computeDescriptorSet,
		.dstBinding = descriptorData->data.binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorData->descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = &descBufferInfo,
		.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanSplinePipelineModule::InitializeGraphicsPipeline(const SplineEntityData& splineEntity, const VkRenderPass renderPass)
{
	m_program = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>(splineEntity.splineRenderer->GetMaterial()->GetProgram());
	m_offset = std::vector<UInt32>(m_program->GetReflection().GetDynamicDescriptorCount(), 0);
	m_material = splineEntity.material;
	m_widthOffset = m_program->GetReflection().GetPushConstantBlockData("_width")->offset;

	VkPipelineInputAssemblyStateCreateInfo pInputAssemblyInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
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
