#include "vulkan-pch.h"
#include "VulkanSpikeMeshRendererSetup.h"
#include "Core/VulkanPipelineFactory.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanMeshController.h"
#include "Mesh/VulkanMeshPipelineModule.h"
#include <Core/GameEntity.h>
#include <Core/Mesh.h>
#include <Core/Logger.h>
#include <Rendering/Material.h>
#include <Rendering/SpikeMeshRenderer.h>
#include <Rendering/ShaderInternalNames.h>
#include <map>

namespace LuxonEngine::Rendering::Vulkan {
	void VulkanSpikeMeshRendererSetup::Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context)
	{
		Teardown();

		struct SpikeData {
			GameEntity* entity;
			VkBuffer transformBuffer;
			VulkanMeshController* meshController;
			SpikeMeshRenderer* spikeRenderer;
		};

		std::map<Material*, std::vector<SpikeData>> spikeMap;

		for (auto& rendererEntity : entities) {
			auto* spikeRenderer = RendererCast<SpikeMeshRenderer>(rendererEntity.renderer);

			if (spikeRenderer == nullptr || spikeRenderer->GetMaterial() == nullptr || spikeRenderer->GetMesh() == nullptr)
				continue;

			auto* meshController = dynamic_cast<VulkanMeshController*>(spikeRenderer->GetMesh()->GetGPUHandle().get());

			if (meshController == nullptr)
				continue;

			spikeMap[spikeRenderer->GetMaterial().get()].push_back(SpikeData{
				.entity = rendererEntity.entity,
				.transformBuffer = rendererEntity.transformBuffer,
				.meshController = meshController,
				.spikeRenderer = spikeRenderer,
				});
		}

		constexpr UInt32 trianglesPerTaskGroup = 32 * 32;

		std::string error;
		MeshShading::MeshPipelineProperties properties;

		for (auto& [material, spikeList] : spikeMap) {
			auto pipeline = context.pipelineFactory->CreateMeshPipeline(material, context.mainRenderPass->GetRenderPass(), properties, error);

			if (pipeline == nullptr) {
				Logger::LogError("Failed to create the spike mesh pipeline: " + error);
				continue;
			}

			if (pipeline->Initialize((UInt32)spikeList.size()) == false) {
				Logger::LogError("Failed to create the descriptors of the spike mesh pipeline");
				continue;
			}

			pipeline->SetDescriptor(INTERNAL_CAMERA_DATA_NAME, context.cameraBuffer, 0, context.cameraStride);
			pipeline->SetDescriptor(INTERNAL_LIGHT_DATA_NAME, context.lightBuffer, 0, context.lightStride);

			for (auto& spikeData : spikeList) {
				pipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_OBJECT_TRANSFORM_DATA_NAME, spikeData.transformBuffer);

				VkBuffer vertexStorageBuffer = spikeData.meshController->CreateVertexStorageBuffer();
				VkBuffer indexStorageBuffer = spikeData.meshController->CreateIndexStorageBuffer();
				context.meshStorageBuffers->push_back(vertexStorageBuffer);
				context.meshStorageBuffers->push_back(indexStorageBuffer);

				pipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_VERTEX_BUFFER_NAME, vertexStorageBuffer);
				pipeline->SetEntityDescriptor(spikeData.entity, INTERNAL_INDEX_BUFFER_NAME, indexStorageBuffer);
				pipeline->SetEntityConstant(spikeData.entity, "_height", spikeData.spikeRenderer->GetSpikeHeight());

				UInt32 triangleCount = spikeData.spikeRenderer->GetMesh()->GetIndexCount() / 3;
				pipeline->SetEntityThreadGroupCount(spikeData.entity, (triangleCount + trianglesPerTaskGroup - 1) / trianglesPerTaskGroup);
			}

			context.mainRenderPass->AddPipeline(pipeline.get());
			m_pipelines.push_back(pipeline);
		}
	}

	void VulkanSpikeMeshRendererSetup::Teardown()
	{
		m_pipelines.clear();
	}
}
