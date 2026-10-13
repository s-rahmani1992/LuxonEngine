#include "vulkan-pch.h"
#include "VulkanPipelineFactory.h"
#include "Mesh/VulkanMeshPipelineModule.h"
#include "Mesh/SPIRVMeshProgram.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Compute/VulkanComputePipelineModule.h"
#include "Compute/SPIRVComputeProgram.h"
#include "RayTracing/VulkanRayTracingPipelineModule.h"
#include "RayTracing/SPIRVRayTracingProgram.h"
#include "Rendering/Material.h"
#include <vector>

namespace LuxonEngine::Rendering::Vulkan {
	VulkanPipelineFactory::VulkanPipelineFactory(VkDevice device)
		:m_device(device)
	{

	}

	ref<RayTracing::VulkanRayTracingPipelineModule> VulkanPipelineFactory::CreateRayTracingPipeline(const ref<Material>& globalMaterial, const std::vector<ref<GameEntity>>& entities, std::string& error)
	{
		if (globalMaterial == nullptr) {
			error = "Material is null";
			return nullptr;
		}

		auto globalProgram = std::dynamic_pointer_cast<RayTracing::SPIRVRayTracingProgram>(globalMaterial->GetProgram());

		if (globalProgram == nullptr || globalProgram->IsGlobal() == false) {
			error = "Program is not a global ray tracing shader program";
			return nullptr;
		}

		auto rayTracingPipeline = std::make_shared<RayTracing::VulkanRayTracingPipelineModule>();

		if (rayTracingPipeline->Initialize(entities, globalMaterial) == false) {
			error = "Failed to initialize the ray tracing pipeline";
			return nullptr;
		}

		return rayTracingPipeline;
	}

	ref<Compute::VulkanComputePipelineModule> VulkanPipelineFactory::CreateComputePipeline(Material* material, std::string& error)
	{
		if (material == nullptr) {
			error = "Material is null";
			return nullptr;
		}

		return CreateComputePipeline(material->GetProgram().get(), material, error);
	}

	ref<Compute::VulkanComputePipelineModule> VulkanPipelineFactory::CreateComputePipeline(const ShaderProgram* program, std::string& error)
	{
		return CreateComputePipeline(program, nullptr, error);
	}

	ref<Compute::VulkanComputePipelineModule> VulkanPipelineFactory::CreateComputePipeline(const ShaderProgram* program, Material* material, std::string& error)
	{
		auto computeProgram = dynamic_cast<const Compute::SPIRVComputeProgram*>(program);

		if (computeProgram == nullptr) {
			error = "Program is not a compute shader program";
			return nullptr;
		}

		if (computeProgram->GetPipelineLayout() == VK_NULL_HANDLE) {
			error = "Pipeline layout of the compute program is not created";
			return nullptr;
		}

		VkComputePipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = computeProgram->GetStageInfo(),
			.layout = computeProgram->GetPipelineLayout(),
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		VkPipeline pipeline = VK_NULL_HANDLE;

		if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
			error = "Failed to create compute pipeline";
			return nullptr;
		}

		return std::make_shared<Compute::VulkanComputePipelineModule>(m_device, pipeline, computeProgram, material);
	}

	ref<Rasterization::VulkanRasterizationPipelineModule> VulkanPipelineFactory::CreateRasterizationPipeline(Material* material, VkRenderPass renderPass, const Rasterization::RasterizationPipelineProperties& properties, std::string& error)
	{
		if (material == nullptr) {
			error = "Material is null";
			return nullptr;
		}

		return CreateRasterizationPipeline(material->GetProgram().get(), material, renderPass, properties, error);
	}

	ref<Rasterization::VulkanRasterizationPipelineModule> VulkanPipelineFactory::CreateRasterizationPipeline(const ShaderProgram* program, VkRenderPass renderPass, const Rasterization::RasterizationPipelineProperties& properties, std::string& error)
	{
		return CreateRasterizationPipeline(program, nullptr, renderPass, properties, error);
	}

	ref<Rasterization::VulkanRasterizationPipelineModule> VulkanPipelineFactory::CreateRasterizationPipeline(const ShaderProgram* program, Material* material, VkRenderPass renderPass, const Rasterization::RasterizationPipelineProperties& properties, std::string& error)
	{
		auto rasterizationProgram = dynamic_cast<const Rasterization::SPIRVRasterizationProgram*>(program);

		if (rasterizationProgram == nullptr) {
			error = "Program is not a rasterization shader program";
			return nullptr;
		}

		if (rasterizationProgram->GetPipelineLayout() == VK_NULL_HANDLE) {
			error = "Pipeline layout of the rasterization program is not created";
			return nullptr;
		}

		switch (properties.topology) {
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
			break;
		default:
			error = "Unsupported primitive topology of the rasterization pipeline";
			return nullptr;
		}

		if (properties.renderTargetFormats.size() > 8) {
			error = "Too many render target formats in the rasterization pipeline properties";
			return nullptr;
		}

		auto& inputAssembly = rasterizationProgram->GetInputAssemblyReflection();

		VkVertexInputBindingDescription bindingDescription{
			.binding = 0,
			.stride = inputAssembly.stride,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = inputAssembly.attributes.empty() ? 0u : 1u,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = (UInt32)inputAssembly.attributes.size(),
			.pVertexAttributeDescriptions = inputAssembly.attributes.empty() ? nullptr : inputAssembly.attributes.data(),
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = properties.topology,
			.primitiveRestartEnable = VK_FALSE,
		};

		GraphicsPipelineState state{
			.stages = &rasterizationProgram->GetStageInfos(),
			.layout = rasterizationProgram->GetPipelineLayout(),
			.renderPass = renderPass,
			.vertexInput = &vertexInputInfo,
			.inputAssembly = &inputAssemblyInfo,
			.enableDepthTest = properties.enableDepthTest,
			.cullMode = properties.cullMode,
			.renderTargetCount = properties.renderTargetFormats.empty() ? (UInt32)rasterizationProgram->GetRenderTargetReflection().formats.size() : (UInt32)properties.renderTargetFormats.size(),
		};

		VkPipeline pipeline = CreateGraphicsPipeline(state, error);

		if (pipeline == VK_NULL_HANDLE)
			return nullptr;

		return std::make_shared<Rasterization::VulkanRasterizationPipelineModule>(m_device, pipeline, rasterizationProgram, material);
	}

	VkPipeline VulkanPipelineFactory::CreateGraphicsPipeline(const GraphicsPipelineState& state, std::string& error)
	{
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
			.cullMode = state.cullMode,
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
			.depthTestEnable = state.enableDepthTest ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = state.enableDepthTest ? VK_TRUE : VK_FALSE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
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

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(state.renderTargetCount, colorBlendAttachmentState);

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

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = (UInt32)state.stages->size(),
			.pStages = state.stages->data(),
			.pVertexInputState = state.vertexInput,
			.pInputAssemblyState = state.inputAssembly,
			.pTessellationState = nullptr,
			.pViewportState = &viewportStateInfo,
			.pRasterizationState = &rasterizationStateInfo,
			.pMultisampleState = &multisampleStateInfo,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendStateInfo,
			.pDynamicState = &dynamicStateInfo,
			.layout = state.layout,
			.renderPass = state.renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		VkPipeline pipeline = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
			error = "Failed to create graphics pipeline";
			return VK_NULL_HANDLE;
		}

		return pipeline;
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

		GraphicsPipelineState state{
			.stages = &meshProgram->GetStageInfos(),
			.layout = meshProgram->GetPipelineLayout(),
			.renderPass = renderPass,
			.vertexInput = nullptr,
			.inputAssembly = nullptr,
			.enableDepthTest = properties.enableDepthTest,
			.cullMode = VK_CULL_MODE_NONE,
			.renderTargetCount = meshProgram->GetRenderTargetCount(),
		};

		VkPipeline pipeline = CreateGraphicsPipeline(state, error);

		if (pipeline == VK_NULL_HANDLE)
			return nullptr;

		return std::make_shared<MeshShading::VulkanMeshPipelineModule>(m_device, pipeline, meshProgram, material);
	}
}
