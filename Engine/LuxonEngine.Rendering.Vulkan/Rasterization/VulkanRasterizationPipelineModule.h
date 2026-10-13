#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"
#include "Core/VulkanMaterialResourceManager.h"
#include <memory>
#include <vector>

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::Vulkan::Rasterization {
	class SPIRVRasterizationProgram;

	class VulkanRasterizationPipelineModule {
	public:
		VulkanRasterizationPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVRasterizationProgram* program, Material* material);

		~VulkanRasterizationPipelineModule();

		VulkanRasterizationPipelineModule(const VulkanRasterizationPipelineModule&) = delete;
		VulkanRasterizationPipelineModule& operator=(const VulkanRasterizationPipelineModule&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		bool SetEntityGeometry(GameEntity* entity, VkBuffer vertexBuffer, VkBuffer indexBuffer, UInt32 indexCount);

		bool SetEntityGeometry(GameEntity* entity, VkBuffer vertexBuffer, UInt32 vertexCount);

		void BindGlobal(VkCommandBuffer commandBuffer);

		bool DrawEntity(VkCommandBuffer commandBuffer, GameEntity* entity);

		void Draw(VkCommandBuffer commandBuffer);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return m_resourceManager->SetEntityConstant(entity, name, value);
		}

		inline VkPipeline GetPipeline() const { return m_pipeline; }
		inline const SPIRVRasterizationProgram* GetProgram() const { return m_program; }
		inline Material* GetMaterial() const { return m_material; }

	private:
		struct EntityGeometry {
			VkBuffer vertexBuffer = VK_NULL_HANDLE;
			VkBuffer indexBuffer = VK_NULL_HANDLE;
			UInt32 indexCount = 0;
			UInt32 vertexCount = 0;
		};

		bool DrawSlot(VkCommandBuffer commandBuffer, UInt32 slot);

		VkDevice m_device;
		VkPipeline m_pipeline;
		const SPIRVRasterizationProgram* m_program;
		Material* m_material;

		std::unique_ptr<VulkanMaterialResourceManager> m_resourceManager;
		std::vector<EntityGeometry> m_entityGeometries;
	};
}
