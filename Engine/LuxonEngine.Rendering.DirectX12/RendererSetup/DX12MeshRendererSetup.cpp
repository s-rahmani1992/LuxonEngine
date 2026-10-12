#include "pch.h"
#include "DX12MeshRendererSetup.h"
#include "Core/DX12PipelineFactory.h"
#include "Core/DX12RenderPass.h"
#include "Rasterization/DX12RasterizationPipelineModule.h"
#include "DX12GraphicContext.h"
#include "DX12MeshController.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/MeshRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::DX12 {
	void DX12MeshRendererSetup::Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		struct MeshEntityData {
			GameEntity* entity;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
			ref<DX12MeshController> meshController;
		};

		std::map<ref<Material>, std::vector<MeshEntityData>> materialMap;

		// group the entities by their material, one pipeline is created per material
		for (auto& rendererEntity : entities) {
			auto meshRenderer = RendererCast<MeshRenderer>(rendererEntity.renderer);

			if (meshRenderer == nullptr || meshRenderer->GetMesh() == nullptr)
				continue;

			auto material = meshRenderer->GetMaterial();

			if (material == nullptr) {
				Logger::LogWarning("The mesh renderer of " + rendererEntity.entity->GetName() + " has no material and is not drawn");
				continue;
			}

			auto meshController = std::dynamic_pointer_cast<DX12MeshController>(meshRenderer->GetMesh()->GetGPUHandle());
			if (meshController == nullptr) // the mesh is not uploaded to the GPU
				continue;

			materialMap[material].push_back(MeshEntityData{
				.entity = rendererEntity.entity,
				.transformHandle = rendererEntity.gpuData->transformHeap->GetCPUDescriptorHandleForHeapStart(),
				.meshController = meshController,
				});
		}

		std::string error;
		Rasterization::RasterizationPipelineProperties properties;

		for (auto& [material, entityList] : materialMap) {
			auto pipeline = context.pipelineFactory->CreateRasterizationPipeline(material.get(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the rasterization pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)entityList.size()) == false)
				continue;

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightDescriptor);

			for (auto& globalDescriptor : context.globalDescriptors)
				pipeline->SetDescriptor(globalDescriptor.name, globalDescriptor.handle);

			for (auto& entityData : entityList) {
				pipeline->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformHandle);
				pipeline->SetEntityGeometry(entityData.entity, *entityData.meshController->GetVertexView(),
					*entityData.meshController->GetIndexView(), entityData.meshController->GetMesh()->GetIndexCount());
			}

			context.mainRenderPass->AddPipeline(pipeline);
		}
	}
}
