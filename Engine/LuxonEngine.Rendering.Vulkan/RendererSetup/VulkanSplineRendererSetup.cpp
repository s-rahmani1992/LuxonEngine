#include "vulkan-pch.h"
#include "VulkanSplineRendererSetup.h"
#include "Core/VulkanPipelineFactory.h"
#include "Core/VulkanBufferFactory.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanComputePass.h"
#include "Core/VulkanSplineVertex.h"
#include "Compute/VulkanComputePipelineModule.h"
#include "Compute/SPIRVComputeProgram.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include <Core/GameEntity.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/SplineRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <algorithm>
#include <map>

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanSplineRendererSetup::Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		Teardown();

		for (auto& rendererEntity : entities) {
			auto* splineRenderer = RendererCast<SplineRenderer>(rendererEntity.renderer);

			if (splineRenderer == nullptr || splineRenderer->GetSegments() <= 0)
				continue;

			if (splineRenderer->GetMaterial() == nullptr) {
				Logger::LogWarning("The spline renderer of " + rendererEntity.entity->GetName() + " has no material and is not drawn");
				continue;
			}

			SplineGPUData spline;
			spline.renderer = splineRenderer;
			spline.entity = rendererEntity.entity;
			spline.transformBuffer = rendererEntity.transformBuffer;
			spline.vertexCount = (UInt32)splineRenderer->GetSegments() + 1;

			spline.vertexBuffer = context.bufferFactory->CreateStructuredBuffer<SplineVertex>(VulkanBufferDesc{
				.memoryType = VulkanMemoryType::GPUOnly,
				.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			}, (int)spline.vertexCount);

			if (spline.vertexBuffer == nullptr) {
				Logger::LogError("Failed to create the vertex buffer of " + rendererEntity.entity->GetName());
				continue;
			}

			m_splines.push_back(std::move(spline));
		}

		if (m_splines.empty())
			return;

		auto* computeProgram = context.getInternalProgram ? context.getInternalProgram("Bezier_Curve_Compute_Program") : nullptr;

		if (computeProgram == nullptr) {
			Logger::LogError("The spline compute program is not registered, the splines are not drawn");
			Teardown();
			return;
		}

		std::string error;
		m_computeModule = context.pipelineFactory->CreateComputePipeline(computeProgram, error);

		if (m_computeModule == nullptr) {
			Logger::LogError("Failed to create the spline compute pipeline: " + error);
			Teardown();
			return;
		}

		if (m_computeModule->Initialize((UInt32)m_splines.size()) == false) {
			Logger::LogError("Failed to create the descriptors of the spline compute pipeline");
			Teardown();
			return;
		}

		UInt32 threadsPerGroup = (std::max)(1u, (UInt32)m_computeModule->GetProgram()->GetThreadGroupReflection().xThread);
		std::map<Material*, std::vector<size_t>> materialMap;

		for (size_t i = 0; i < m_splines.size(); i++) {
			auto& spline = m_splines[i];

			m_computeModule->SetEntityDescriptor(spline.entity, INTERNAL_VERTEX_BUFFER_NAME, spline.vertexBuffer->GetBuffer());
			m_computeModule->SetEntityThreadGroupCount(spline.entity, (spline.vertexCount + threadsPerGroup - 1) / threadsPerGroup);

			context.computePass->Use(spline.vertexBuffer.get(), VulkanStates::Storage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
			context.mainRenderPass->Use(spline.vertexBuffer.get(), VulkanStates::VertexInput());

			spline.renderer->SetDirty();

			materialMap[spline.renderer->GetMaterial().get()].push_back(i);
		}

		context.computePass->AddPipeline(m_computeModule.get());

		Rasterization::RasterizationPipelineProperties properties;
		properties.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		properties.cullMode = VK_CULL_MODE_NONE;

		for (auto& [material, splineIndices] : materialMap) {
			auto pipeline = context.pipelineFactory->CreateRasterizationPipeline(material, context.mainRenderPass->GetRenderPass(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the spline rasterization pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)splineIndices.size()) == false) {
				Logger::LogError("Failed to create the descriptors of the spline rasterization pipeline");
				continue;
			}

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer, 0, context.cameraStride);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightBuffer, 0, context.lightStride);

			for (auto splineIndex : splineIndices) {
				auto& spline = m_splines[splineIndex];

				pipeline->SetEntityDescriptor(spline.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, spline.transformBuffer);
				pipeline->SetEntityGeometry(spline.entity, spline.vertexBuffer->GetBuffer(), spline.vertexCount);
				spline.rasterModule = pipeline.get();
			}

			context.mainRenderPass->AddPipeline(pipeline.get());
			m_pipelines.push_back(pipeline);
		}
	}

	void VulkanSplineRendererSetup::Update()
	{
		if (m_computeModule == nullptr)
			return;

		for (auto& spline : m_splines) {
			if (spline.renderer->IsDirty() == false)
				continue;

			auto& curve = spline.renderer->GetCurve();
			float tileFactor = spline.renderer->GetTileFactor();
			float length = curve.InterpolateLength(1.0f);

			m_computeModule->SetEntityConstant(spline.entity, "_startPoint", curve.m_point1);
			m_computeModule->SetEntityConstant(spline.entity, "_midPoint", curve.m_point2);
			m_computeModule->SetEntityConstant(spline.entity, "_endPoint", curve.m_point3);
			m_computeModule->SetEntityConstant(spline.entity, "_tileFactor", tileFactor);
			m_computeModule->SetEntityConstant(spline.entity, "_length", length);

			if (spline.rasterModule != nullptr) {
				float width = spline.renderer->GetWidth();
				spline.rasterModule->SetEntityConstant(spline.entity, INTERNAL_SPLINE_WIDTH_NAME, width);
			}
		}
	}

	void VulkanSplineRendererSetup::Teardown()
	{
		m_pipelines.clear();
		m_computeModule.reset();
		m_splines.clear();
	}
}
