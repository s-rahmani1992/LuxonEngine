#include "vulkan-pch.h"
#include "VulkanMeshRendererSetup.h"
#include "Core/VulkanPipelineFactory.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanMeshController.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/MeshRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanMeshRendererSetup::Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		Teardown();

		struct MeshEntityData {
			GameEntity* entity;
			VkBuffer transformBuffer;
			VulkanMeshController* meshController;
			UInt32 indexCount;
		};

		std::map<Material*, std::vector<MeshEntityData>> materialMap;

		for (auto& rendererEntity : entities) {
			auto* meshRenderer = RendererCast<MeshRenderer>(rendererEntity.renderer);

			if (meshRenderer == nullptr || meshRenderer->GetMesh() == nullptr)
				continue;

			auto material = meshRenderer->GetMaterial();

			if (material == nullptr) {
				Logger::LogWarning("The mesh renderer of " + rendererEntity.entity->GetName() + " has no material and is not drawn");
				continue;
			}

			auto mesh = meshRenderer->GetMesh();
			auto* meshController = dynamic_cast<VulkanMeshController*>(mesh->GetGPUHandle().get());

			if (meshController == nullptr)
				continue;

			materialMap[material.get()].push_back(MeshEntityData{
				.entity = rendererEntity.entity,
				.transformBuffer = rendererEntity.transformBuffer,
				.meshController = meshController,
				.indexCount = mesh->GetIndexCount(),
				});
		}

		std::string error;
		Rasterization::RasterizationPipelineProperties properties;

		for (auto& [material, entityList] : materialMap) {
			auto pipeline = context.pipelineFactory->CreateRasterizationPipeline(material, context.mainRenderPass->GetRenderPass(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the rasterization pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)entityList.size()) == false) {
				Logger::LogError("Failed to create the descriptors of the rasterization pipeline");
				continue;
			}

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer, 0, context.cameraStride);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightBuffer, 0, context.lightStride);

			for (auto& globalDescriptor : context.globalDescriptors)
				pipeline->SetDescriptor(globalDescriptor.name, globalDescriptor.imageView);

			for (auto& entityData : entityList) {
				pipeline->SetEntityDescriptor(entityData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, entityData.transformBuffer);
				pipeline->SetEntityGeometry(entityData.entity, entityData.meshController->GetVertexBuffer(), entityData.meshController->GetIndexBuffer(), entityData.indexCount);
			}

			context.mainRenderPass->AddPipeline(pipeline.get());
			m_pipelines.push_back(pipeline);
		}
	}

	void VulkanMeshRendererSetup::Teardown()
	{
		m_pipelines.clear();
	}
}
