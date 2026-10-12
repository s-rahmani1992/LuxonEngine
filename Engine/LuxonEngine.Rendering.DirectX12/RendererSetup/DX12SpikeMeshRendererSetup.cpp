#include "pch.h"
#include "DX12SpikeMeshRendererSetup.h"
#include "Core/DX12PipelineFactory.h"
#include "Core/DX12RenderPass.h"
#include "Mesh/DX12MeshPipelineModule.h"
#include "DX12MeshController.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/SpikeMeshRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::DX12 {
	void DX12SpikeMeshRendererSetup::Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		struct SpikeData {
			GameEntity* entity;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
			ref<DX12MeshController> meshController;
			SpikeMeshRenderer* spikeRenderer;
		};

		std::map<ref<Material>, std::vector<SpikeData>> spikeMap;

		// group the spike entities by material, one mesh pipeline is created per material
		for (auto& rendererEntity : entities) {
			auto spikeRenderer = RendererCast<SpikeMeshRenderer>(rendererEntity.renderer);

			if (spikeRenderer == nullptr || spikeRenderer->GetMaterial() == nullptr || spikeRenderer->GetMesh() == nullptr)
				continue;

			auto meshController = std::dynamic_pointer_cast<DX12MeshController>(spikeRenderer->GetMesh()->GetGPUHandle());
			if (meshController == nullptr) // the mesh is not uploaded to the GPU
				continue;

			spikeMap[spikeRenderer->GetMaterial()].push_back(SpikeData{
				.entity = rendererEntity.entity,
				.transformHandle = rendererEntity.gpuData->transformHeap->GetCPUDescriptorHandleForHeapStart(),
				.meshController = meshController,
				.spikeRenderer = spikeRenderer,
				});
		}

		// the pyramid shader handles 32 mesh groups of 32 triangles in each task group
		constexpr UInt32 trianglesPerTaskGroup = 32 * 32;
		MeshShading::MeshPipelineProperties properties;
		std::string error;

		for (auto& [material, spikeList] : spikeMap) {
			ref<MeshShading::DX12MeshPipelineModule> pipeline = context.pipelineFactory->CreateMeshPipeline(material.get(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the spike mesh pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)spikeList.size()) == false)
				continue;

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightDescriptor);

			for (auto& spikeData : spikeList) {
				pipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, spikeData.transformHandle);
				pipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_VERTEX_BUFFER_NAME, spikeData.meshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
				pipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_INDEX_BUFFER_NAME, spikeData.meshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
				pipeline->SetEntityConstant(spikeData.entity, "_height", spikeData.spikeRenderer->GetSpikeHeight());

				// every entity launches only the task groups its own mesh needs
				UInt32 triangleCount = spikeData.spikeRenderer->GetMesh()->GetIndexCount() / 3;
				pipeline->SetEntityThreadGroupCount(spikeData.entity, (triangleCount + trianglesPerTaskGroup - 1) / trianglesPerTaskGroup);
			}

			context.mainRenderPass->AddPipeline(pipeline);
		}
	}
}
