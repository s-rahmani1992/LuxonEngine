#include "vulkan-pch.h"
#include "VulkanGBufferPipelineModule.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanBufferFactory.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Core/VulkanHybridContext.h"
#include "Core/Mesh.h"
#include "Core/GameEntity.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include "Core/VulkanMeshController.h"
#include "Core/VulkanUtilities.h"

QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::VulkanGBufferPipelineModule()
	:m_device(VulkanDeviceManager::Instance()->GetGraphicDevice()),
	m_positionFormat(VK_FORMAT_R16G16B16A16_SFLOAT), 
	m_normalFormat(VK_FORMAT_R16G16B16A16_UNORM), 
	m_maskFormat(VK_FORMAT_R32_UINT)
{
}

QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::~VulkanGBufferPipelineModule()
{
	vkDestroyImageView(m_device, m_positionImageView, nullptr);
	vkDestroyImage(m_device, m_positionImage, nullptr);
	vkFreeMemory(m_device, m_positionImageMemory, nullptr);

	vkDestroyImageView(m_device, m_normalImageView, nullptr);
	vkDestroyImage(m_device, m_normalImage, nullptr);
	vkFreeMemory(m_device, m_normalImageMemory, nullptr);

	vkDestroyImageView(m_device, m_maskImageView, nullptr);
	vkDestroyImage(m_device, m_maskImage, nullptr);
	vkFreeMemory(m_device, m_maskImageMemory, nullptr);

	vkDestroyFramebuffer(m_device, m_frameBuffer, nullptr);

	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::InitializePipeline(const std::vector<VKEntityGPUData>& entities, const ref<Rasterization::SPIRVRasterizationProgram>& gBufferProgram, UInt32 width, UInt32 height, VkImageView depthView)
{
	m_gBufferProgram = gBufferProgram;
	m_depthView = depthView;
	m_offsets = std::vector<UInt32>(gBufferProgram->GetReflection().GetDynamicDescriptorCount(), 0);
	auto descriptorData = gBufferProgram->GetReflection().GetDescriptorData(HLSL_OBJECT_TRANSFORM_DATA_NAME);
	m_transformIndex = descriptorData->offsetIndex;
	
	if(CreateRenderPass() == false)
		return false;

	if(CreateFrameBuffers(width, height) == false)
		return false;

	if(CreateRasterPipeline() == false)
		return false;

	if(CreateDescriptorSets(gBufferProgram) == false)
		return false;

	for (auto& entity : entities) {
		m_entities.push_back(GBufferEntityGPUData{
			.meshController = std::dynamic_pointer_cast<VulkanMeshController>(entity.gameEntity->GetRenderer()->GetMesh()->GetGPUHandle()),
			.indexCount = entity.gameEntity->GetRenderer()->GetMesh()->GetIndexCount(),
			.transformOffset = (UInt32)sizeof(TransformGPU) * entity.index,
			});
	}

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::WriteBuffer(const std::string& name, VkBuffer buffer, UInt32 stride)
{
	auto descriptorData = m_gBufferProgram->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	VkDescriptorBufferInfo descBufferInfo{
		.buffer = buffer,
		.offset = 0,
		.range = stride,
	};

	VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptorSets[descriptorData->data.set],
		.dstBinding = descriptorData->data.binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorData->descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = &descBufferInfo,
		.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
}

void QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::RenderCommand(VkCommandBuffer commandBuffer)
{
	vkCmdBeginRenderPass(commandBuffer, &m_renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gBufferPipeline);
	VkDeviceSize offsets[] = { 0 };

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_width;
	viewport.height = (float)m_height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { m_width, m_height };

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	for (auto& entity : m_entities) {
		auto indexBuffer = entity.meshController->GetIndexBuffer();
		auto vertexBuffer = entity.meshController->GetVertexBuffer();
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);		
		m_offsets[m_transformIndex] = entity.transformOffset;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gBufferProgram->GetPipelineLayout(), 0, (UInt32)m_descriptorSets.size(), m_descriptorSets.data(), m_offsets.size(), m_offsets.data());
		vkCmdDrawIndexed(commandBuffer, entity.indexCount, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(commandBuffer);
}

bool QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::CreateRenderPass()
{
	VkAttachmentDescription attachments[4] = {};

	attachments[0] = {
		.format = m_positionFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	attachments[1] = {
		.format = m_normalFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	attachments[2] = {
		.format = m_maskFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	attachments[3] = {
		.format = VK_FORMAT_D32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference colorRefs[3] = {
		{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
		{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
		{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
	};

	VkAttachmentReference depthAttachmentRef{
		.attachment = 3,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 3,
		.pColorAttachments = colorRefs,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	VkRenderPassCreateInfo rpInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 4,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	vkCreateRenderPass(m_device, &rpInfo, nullptr, &m_renderPass);

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::CreateFrameBuffers(UInt32 width, UInt32 height)
{
	m_width = width;
	m_height = height;
	auto bufferFactory = VulkanDeviceManager::Instance()->GetBufferFactory();

	// Position buffer
	VkImageCreateInfo imgInfo{};
	imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.format = m_positionFormat;
	imgInfo.extent = { width, height, 1 };
	imgInfo.mipLevels = 1;
	imgInfo.arrayLayers = 1;
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | 
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT;

	bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_positionImage, &m_positionImageMemory);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_positionImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imgInfo.format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(m_device, &viewInfo, nullptr, &m_positionImageView);

	//normalBuffer

	imgInfo.format = m_normalFormat;
	bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_normalImage, &m_normalImageMemory);

	viewInfo.format = m_normalFormat;
	viewInfo.image = m_normalImage;

	vkCreateImageView(m_device, &viewInfo, nullptr, &m_normalImageView);

	// Mask Buffer
	imgInfo.format = m_maskFormat;
	bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_maskImage, &m_maskImageMemory);

	viewInfo.format = m_maskFormat;
	viewInfo.image = m_maskImage;

	vkCreateImageView(m_device, &viewInfo, nullptr, &m_maskImageView);

	VkImageView views[4] = {
	m_positionImageView,
	m_normalImageView,
	m_maskImageView,
	m_depthView,
	};

	// Frame Buffer
	VkFramebufferCreateInfo fbInfo = {
	.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
	.renderPass = m_renderPass,
	.attachmentCount = 4,
	.pAttachments = views,
	.width = width,
	.height = height,
	.layers = 1
	};

	vkCreateFramebuffer(m_device, &fbInfo, nullptr, &m_frameBuffer);

	m_clearValues[0].color = { {1.0f, 1.0f, 1.0f, 1.0f} };
	m_clearValues[1].color = { {1.0f, 1.0f, 1.0f, 1.0f} };
	m_clearValues[2].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
	m_clearValues[3].depthStencil = { 1.0f, 0 };

	m_renderPassInfo = VkRenderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_renderPass,
		.framebuffer = m_frameBuffer,
		.renderArea = {
			.offset = { 0, 0 },
			.extent = {width, height},
		},
		.clearValueCount = 4,
		.pClearValues = m_clearValues,
	};

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::CreateRasterPipeline()
{
	VkVertexInputBindingDescription bindingDescriptions = {
	.binding = 0,
	.stride = sizeof(Vertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	VkVertexInputAttributeDescription attributeDescriptions[3] = {
		{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position) },
		{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
		{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescriptions,
		.vertexAttributeDescriptionCount = 3,
		.pVertexAttributeDescriptions = attributeDescriptions,
	};



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

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState[3];

	colorBlendAttachmentState[0] = VkPipelineColorBlendAttachmentState{
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	colorBlendAttachmentState[2] = colorBlendAttachmentState[1] = colorBlendAttachmentState[0];

	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 3,
		.pAttachments = colorBlendAttachmentState,
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

	auto& stages = m_gBufferProgram->GetStageInfos();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = (UInt32)stages.size(),
		.pStages = stages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &pInputAssemblyInfo,
		.pTessellationState = nullptr,
		.pViewportState = &viewportStateInfo,
		.pRasterizationState = &rasterizationStateInfo,
		.pMultisampleState = &multisampleStateInfo,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendStateInfo,
		.pDynamicState = &dynamicStateInfo,
		.layout = m_gBufferProgram->GetPipelineLayout(),
		.renderPass = m_renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,

	};

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_gBufferPipeline) != VK_SUCCESS) {
		return false;
	}

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanGBufferPipelineModule::CreateDescriptorSets(const ref<Rasterization::SPIRVRasterizationProgram>& gBufferProgram)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(20);

	auto& reflection = gBufferProgram->GetReflection();
	UInt32 setCount = reflection.GetDescriptorLayoutCount();
	auto& descriptors = reflection.GetDescriptors();
	m_descriptorSets.resize(setCount);

	for (auto& descriptor : descriptors) {
		auto it = std::find_if(poolSizes.begin(), poolSizes.end(), [descriptor](const VkDescriptorPoolSize& poolSize) {
			return descriptor.descriptorType == poolSize.type;
			});

		if (it != poolSizes.end())
			(*it).descriptorCount++;
		else
			poolSizes.push_back(VkDescriptorPoolSize{
			.type = descriptor.descriptorType,
			.descriptorCount = 1,
				});
	}

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = setCount,
		.poolSizeCount = (UInt32)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return true;

	VkDescriptorSetAllocateInfo descSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = nullptr,
	};

	auto& descLayouts = gBufferProgram->GetDiscriptorLayouts();

	for (UInt32 i = 0; i < descLayouts.size(); i++) {
		descSetAlloc.pSetLayouts = descLayouts.data() + i;

		if (vkAllocateDescriptorSets(m_device, &descSetAlloc, m_descriptorSets.data() + i) != VK_SUCCESS)
			return false;
	}

	return true;
}
