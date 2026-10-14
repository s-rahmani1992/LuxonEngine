#include "vulkan-pch.h"
#include "VulkanSurfaceInstanceRendererSetup.h"
#include "Core/VulkanPipelineFactory.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanAssetManager.h"
#include "Core/VulkanMeshController.h"
#include "Core/VulkanTexture2DController.h"
#include "Mesh/VulkanMeshPipelineModule.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Texture2D.h>
#include <Core/Transform.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/SurfaceInstanceRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <algorithm>
#include <cmath>
#include <map>

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanSurfaceInstanceRendererSetup::Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		Teardown();

		struct SurfaceInstanceData {
			GameEntity* entity;
			VkBuffer transformBuffer;
			VulkanTexture2DController* maskTexture;
			SurfaceInstanceRenderer* renderer;
		};

		std::map<Material*, std::vector<SurfaceInstanceData>> surfaceInstanceMap;
		std::vector<ref<Mesh>> instanceMeshes;

		for (auto& rendererEntity : entities) {
			auto* renderer = RendererCast<SurfaceInstanceRenderer>(rendererEntity.renderer);

			if (renderer == nullptr || renderer->GetMaterial() == nullptr || renderer->GetNearMesh() == nullptr ||
				renderer->GetMaskTexture() == nullptr || renderer->GetDensity() <= 0.0f)
				continue;

			context.assetManager->UploadTextureToGPU(renderer->GetMaskTexture());

			auto* maskTexture = dynamic_cast<VulkanTexture2DController*>(renderer->GetMaskTexture()->GetGPUHandle().get());

			if (maskTexture == nullptr)
				continue;

			instanceMeshes.push_back(renderer->GetNearMesh());

			if (renderer->GetFarMesh() != nullptr)
				instanceMeshes.push_back(renderer->GetFarMesh());

			surfaceInstanceMap[renderer->GetMaterial().get()].push_back(SurfaceInstanceData{
				.entity = rendererEntity.entity,
				.transformBuffer = rendererEntity.transformBuffer,
				.maskTexture = maskTexture,
				.renderer = renderer,
				});
		}

		if (instanceMeshes.empty() == false)
			context.assetManager->UploadMeshesToGPU(instanceMeshes);

		constexpr UInt32 maxGroupsPerDimension = 65535;

		std::string error;
		MeshShading::MeshPipelineProperties properties;

		for (auto& [material, surfaceInstanceList] : surfaceInstanceMap) {
			auto pipeline = context.pipelineFactory->CreateMeshPipeline(material, context.mainRenderPass->GetRenderPass(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the surface instance pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)surfaceInstanceList.size()) == false) {
				Logger::LogError("Failed to create the descriptors of the surface instance pipeline");
				continue;
			}

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer, 0, context.cameraStride);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightBuffer, 0, context.lightStride);

			for (auto& surfaceInstanceData : surfaceInstanceList) {
				auto* renderer = surfaceInstanceData.renderer;

				auto farMesh = renderer->GetFarMesh() != nullptr ? renderer->GetFarMesh() : renderer->GetNearMesh();
				auto* nearMeshController = dynamic_cast<VulkanMeshController*>(renderer->GetNearMesh()->GetGPUHandle().get());
				auto* farMeshController = dynamic_cast<VulkanMeshController*>(farMesh->GetGPUHandle().get());

				if (nearMeshController == nullptr || farMeshController == nullptr)
					continue;

				VkBuffer nearVertexBuffer = nearMeshController->CreateVertexStorageBuffer();
				VkBuffer nearIndexBuffer = nearMeshController->CreateIndexStorageBuffer();
				VkBuffer farVertexBuffer = farMeshController->CreateVertexStorageBuffer();
				VkBuffer farIndexBuffer = farMeshController->CreateIndexStorageBuffer();
				context.meshStorageBuffers->insert(context.meshStorageBuffers->end(), { nearVertexBuffer, nearIndexBuffer, farVertexBuffer, farIndexBuffer });

				GameEntity* entity = surfaceInstanceData.entity;
				pipeline->SetEntityDescriptor(entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, surfaceInstanceData.transformBuffer);
				pipeline->SetEntityDescriptor(entity, INTERNAL_MASK_TEXTURE_NAME, surfaceInstanceData.maskTexture->GetImageView());
				pipeline->SetEntityDescriptor(entity, INTERNAL_NEAR_VERTEX_BUFFER_NAME, nearVertexBuffer);
				pipeline->SetEntityDescriptor(entity, INTERNAL_NEAR_INDEX_BUFFER_NAME, nearIndexBuffer);
				pipeline->SetEntityDescriptor(entity, INTERNAL_FAR_VERTEX_BUFFER_NAME, farVertexBuffer);
				pipeline->SetEntityDescriptor(entity, INTERNAL_FAR_INDEX_BUFFER_NAME, farIndexBuffer);

				Vector3 planeSize = entity->GetTransform()->Scale();
				float density = renderer->GetDensity();
				UInt32 cellCount[2] = {
					(std::max)(1u, (UInt32)std::ceil(std::abs(planeSize.x) * density)),
					(std::max)(1u, (UInt32)std::ceil(std::abs(planeSize.z) * density)),
				};

				if (cellCount[0] > maxGroupsPerDimension || cellCount[1] > maxGroupsPerDimension) {
					Logger::LogWarning("The surface instance grid of " + entity->GetName() + " is clamped to 65535 cells per axis");
					cellCount[0] = (std::min)(cellCount[0], maxGroupsPerDimension);
					cellCount[1] = (std::min)(cellCount[1], maxGroupsPerDimension);
				}

				pipeline->SetEntityConstant(entity, "_step", 1.0f / density);
				pipeline->SetEntityConstant(entity, "_totalIndices", cellCount);
				pipeline->SetEntityConstant(entity, "_entityScale", renderer->GetInstanceScale());
				pipeline->SetEntityConstant(entity, "_distance", renderer->GetNearDistance());
				pipeline->SetEntityThreadGroupCount(entity, cellCount[0], cellCount[1]);
			}

			context.mainRenderPass->AddPipeline(pipeline.get());
			m_pipelines.push_back(pipeline);
		}
	}

	void VulkanSurfaceInstanceRendererSetup::Teardown()
	{
		m_pipelines.clear();
	}
}
