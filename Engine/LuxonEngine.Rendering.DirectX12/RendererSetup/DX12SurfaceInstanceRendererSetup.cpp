#include "pch.h"
#include "DX12SurfaceInstanceRendererSetup.h"
#include "Core/DX12PipelineFactory.h"
#include "Core/DX12RenderPass.h"
#include "Mesh/DX12MeshPipelineModule.h"
#include "DX12AssetManager.h"
#include "DX12MeshController.h"
#include "DX12Texture2DController.h"
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

namespace LuxonEngine::Rendering::DX12 {
	void DX12SurfaceInstanceRendererSetup::Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context)
	{
		struct SurfaceInstanceData {
			GameEntity* entity;
			D3D12_CPU_DESCRIPTOR_HANDLE transformHandle;
			ref<DX12Texture2DController> maskTexture;
			SurfaceInstanceRenderer* surfaceInstanceRenderer;
		};

		std::map<ref<Material>, std::vector<SurfaceInstanceData>> surfaceInstanceMap;
		std::vector<ref<Mesh>> instanceMeshes;

		// group the surface instance entities by material, one mesh pipeline is created per material
		for (auto& rendererEntity : entities) {
			auto surfaceInstanceRenderer = RendererCast<SurfaceInstanceRenderer>(rendererEntity.renderer);

			if (surfaceInstanceRenderer == nullptr || surfaceInstanceRenderer->GetMaterial() == nullptr || surfaceInstanceRenderer->GetNearMesh() == nullptr ||
				surfaceInstanceRenderer->GetMaskTexture() == nullptr || surfaceInstanceRenderer->GetDensity() <= 0.0f)
				continue;

			// outside the editor the textures are not uploaded on import. already uploaded textures are skipped
			context.assetManager->UploadTextureToGPU(surfaceInstanceRenderer->GetMaskTexture());

			auto maskTexture = std::dynamic_pointer_cast<DX12Texture2DController>(surfaceInstanceRenderer->GetMaskTexture()->GetGPUHandle());
			if (maskTexture == nullptr) // the mask is not uploaded to the GPU
				continue;

			instanceMeshes.push_back(surfaceInstanceRenderer->GetNearMesh());

			if (surfaceInstanceRenderer->GetFarMesh() != nullptr)
				instanceMeshes.push_back(surfaceInstanceRenderer->GetFarMesh());

			surfaceInstanceMap[surfaceInstanceRenderer->GetMaterial()].push_back(SurfaceInstanceData{
				.entity = rendererEntity.entity,
				.transformHandle = rendererEntity.gpuData->transformHeap->GetCPUDescriptorHandleForHeapStart(),
				.maskTexture = maskTexture,
				.surfaceInstanceRenderer = surfaceInstanceRenderer,
				});
		}

		// the base mesh of the renderer is null, so the near and far meshes are not uploaded with the other meshes. already uploaded meshes are skipped
		if (instanceMeshes.empty() == false)
			context.assetManager->UploadMeshesToGPU(instanceMeshes);

		// one task group per cell, every dimension of a DispatchMesh is limited to 65535 groups
		constexpr UInt32 maxGroupsPerDimension = 65535;
		MeshShading::MeshPipelineProperties properties;
		std::string error;

		for (auto& [material, surfaceInstanceList] : surfaceInstanceMap) {
			ref<MeshShading::DX12MeshPipelineModule> pipeline = context.pipelineFactory->CreateMeshPipeline(material.get(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the surface instance pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)surfaceInstanceList.size()) == false)
				continue;

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraDescriptor);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightDescriptor);

			for (auto& surfaceInstanceData : surfaceInstanceList) {
				auto& renderer = surfaceInstanceData.surfaceInstanceRenderer;

				// without a far mesh, the near mesh is bound to both levels
				auto farMesh = renderer->GetFarMesh() != nullptr ? renderer->GetFarMesh() : renderer->GetNearMesh();
				auto nearMeshController = std::dynamic_pointer_cast<DX12MeshController>(renderer->GetNearMesh()->GetGPUHandle());
				auto farMeshController = std::dynamic_pointer_cast<DX12MeshController>(farMesh->GetGPUHandle());

				if (nearMeshController == nullptr || farMeshController == nullptr) // the meshes are not uploaded to the GPU, the entity is not drawn
					continue;

				pipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, surfaceInstanceData.transformHandle);
				pipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_MASK_TEXTURE_NAME, surfaceInstanceData.maskTexture->GetShaderView()->GetCPUDescriptorHandleForHeapStart());
				pipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_NEAR_VERTEX_BUFFER_NAME, nearMeshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
				pipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_NEAR_INDEX_BUFFER_NAME, nearMeshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
				pipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_FAR_VERTEX_BUFFER_NAME, farMeshController->GetVertexSRVHeap()->GetCPUDescriptorHandleForHeapStart());
				pipeline->SetEntityDescriptor(surfaceInstanceData.entity, INTERNAL_FAR_INDEX_BUFFER_NAME, farMeshController->GetIndexSRVHeap()->GetCPUDescriptorHandleForHeapStart());

				// the plane size comes from the x and z scale of the transform, density is the number of cells per unit length
				Vector3 planeSize = surfaceInstanceData.entity->GetTransform()->Scale();
				float density = renderer->GetDensity();
				UInt32 cellCount[2] = {
					std::max(1u, (UInt32)std::ceil(std::abs(planeSize.x) * density)),
					std::max(1u, (UInt32)std::ceil(std::abs(planeSize.z) * density)),
				};

				if (cellCount[0] > maxGroupsPerDimension || cellCount[1] > maxGroupsPerDimension) {
					Logger::LogWarning("The surface instance grid of " + surfaceInstanceData.entity->GetName() + " is clamped to 65535 cells per axis");
					cellCount[0] = std::min(cellCount[0], maxGroupsPerDimension);
					cellCount[1] = std::min(cellCount[1], maxGroupsPerDimension);
				}

				pipeline->SetEntityConstant(surfaceInstanceData.entity, "_step", 1.0f / density);
				pipeline->SetEntityConstant(surfaceInstanceData.entity, "_totalIndices", cellCount);
				pipeline->SetEntityConstant(surfaceInstanceData.entity, "_entityScale", renderer->GetInstanceScale());
				pipeline->SetEntityConstant(surfaceInstanceData.entity, "_distance", renderer->GetNearDistance());
				pipeline->SetEntityThreadGroupCount(surfaceInstanceData.entity, cellCount[0], cellCount[1]);
			}

			context.mainRenderPass->AddPipeline(pipeline);
		}
	}
}
