#include "pch.h"
#include "DX12SplineRendererSetup.h"
#include "Core/DX12PipelineFactory.h"
#include "Core/DX12GPUResourceManager.h"
#include "Core/DX12RenderPass.h"
#include "Core/DX12ComputePass.h"
#include "Compute/DX12ComputePipelineModule.h"
#include "Rasterization/DX12RasterizationPipelineModule.h"
#include "DX12SplineVertex.h"
#include <Core/GameEntity.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/SplineRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::DX12 {
	void DX12SplineRendererSetup::Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		Teardown();
		m_resourceManager = context.resourceManager;

		struct SplineEntry {
			GameEntity* entity;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
			ref<SplineRenderer> renderer;
		};

		std::vector<SplineEntry> splineEntries;

		for (auto& rendererEntity : entities) {
			auto splineRenderer = RendererCast<SplineRenderer>(rendererEntity.renderer);

			if (splineRenderer == nullptr || splineRenderer->GetSegments() <= 0)
				continue;

			if (splineRenderer->GetMaterial() == nullptr) {
				Logger::LogWarning("The spline renderer of " + rendererEntity.entity->GetName() + " has no material and is not drawn");
				continue;
			}

			// the setup keeps the renderer for the updates, so it takes a shared reference from the entity
			splineEntries.push_back(SplineEntry{
				.entity = rendererEntity.entity,
				.transformHandle = rendererEntity.gpuData->transformHeap->GetCPUDescriptorHandleForHeapStart(),
				.renderer = std::static_pointer_cast<SplineRenderer>(rendererEntity.entity->GetRenderer()),
				});
		}

		if (splineEntries.empty())
			return;

		auto computeProgram = context.getInternalProgram ? context.getInternalProgram("Bezier_Curve_Compute_Program") : nullptr;

		if (computeProgram == nullptr) {
			Logger::LogError("The spline compute program is not registered, the splines are not drawn");
			return;
		}

		std::string error;
		m_computeModule = context.pipelineFactory->CreateComputePipeline(computeProgram.get(), error);

		if (m_computeModule == nullptr) {
			Logger::LogError("Failed to create the spline compute pipeline: " + error);
			return;
		}

		if (m_computeModule->Initialize((UInt32)splineEntries.size()) == false) {
			m_computeModule = nullptr;
			return;
		}

		// the thread group size of the compute shader
		constexpr UInt32 threadsPerGroup = 32;
		std::map<ref<Material>, std::vector<size_t>> materialMap; // material of the spline renderer -> indices in m_splines
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> transformHandles; // by index in m_splines

		for (auto& entry : splineEntries) {
			UInt32 vertexCount = (UInt32)entry.renderer->GetSegments() + 1;

			SplineGPUData spline;
			spline.renderer = entry.renderer;
			spline.entity = entry.entity;

			spline.vertexBuffer = context.resourceManager->CreateStructuredBuffer<SplineVertex>(DX12BufferDesc{
				.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				}, vertexCount);

			if (spline.vertexBuffer == nullptr) {
				Logger::LogError("Failed to create the vertex buffer of " + entry.entity->GetName());
				continue;
			}

			m_computeModule->SetEntityDescriptor(spline.entity, INTERNAL_VERTEX_BUFFER_NAME, spline.vertexBuffer->GetWriteHandle());
			m_computeModule->SetEntityThreadGroupCount(spline.entity, (vertexCount + threadsPerGroup - 1) / threadsPerGroup);

			// the vertices are generated every frame: written as a UAV, then read as a vertex buffer by the main render pass
			context.computePass->Use(spline.vertexBuffer.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			context.mainRenderPass->Use(spline.vertexBuffer.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			// the curve parameters are copied to the compute module on the first update
			spline.renderer->SetDirty();

			materialMap[spline.renderer->GetMaterial()].push_back(m_splines.size());
			transformHandles.push_back(entry.transformHandle);
			m_splines.push_back(std::move(spline));
		}

		if (m_splines.empty()) {
			m_computeModule = nullptr;
			return;
		}

		context.computePass->AddPipeline(m_computeModule);

		// the geometry shader turns every line of the strip into a quad, so the quads do not have a fixed winding
		Rasterization::RasterizationPipelineProperties properties;
		properties.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		properties.stripTopology = true;
		properties.cullMode = D3D12_CULL_MODE_NONE;

		for (auto& [material, splineIndices] : materialMap) {
			auto pipeline = context.pipelineFactory->CreateRasterizationPipeline(material.get(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the spline rasterization pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)splineIndices.size()) == false)
				continue;

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightDescriptor);

			for (auto splineIndex : splineIndices) {
				auto& spline = m_splines[splineIndex];
				float width = spline.renderer->GetWidth();

				pipeline->SetEntityDescriptor(spline.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, transformHandles[splineIndex]);
				pipeline->SetEntityGeometry(spline.entity, spline.vertexBuffer->GetVertexBufferView(), (UInt32)spline.renderer->GetSegments() + 1);
				pipeline->SetEntityConstant(spline.entity, INTERNAL_SPLINE_WIDTH_NAME, width);
				spline.rasterModule = pipeline;
			}

			context.mainRenderPass->AddPipeline(pipeline);
		}
	}

	void DX12SplineRendererSetup::Update()
	{
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

	void DX12SplineRendererSetup::Teardown()
	{
		if (m_resourceManager != nullptr) {
			for (auto& spline : m_splines)
				m_resourceManager->Destroy(spline.vertexBuffer);
		}

		m_splines.clear();
		m_computeModule = nullptr;
	}
}
