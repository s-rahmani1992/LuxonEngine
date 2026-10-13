#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"
#include "Core/VulkanMaterialResourceManager.h"
#include <array>
#include <vector>

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::Vulkan::Compute {
	class SPIRVComputeProgram;

	class VulkanComputePipelineModule {
	public:
		VulkanComputePipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVComputeProgram* program, Material* material);

		~VulkanComputePipelineModule();

		VulkanComputePipelineModule(const VulkanComputePipelineModule&) = delete;
		VulkanComputePipelineModule& operator=(const VulkanComputePipelineModule&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		bool SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y = 1, UInt32 z = 1);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return m_resourceManager->SetEntityConstant(entity, name, value);
		}

		void BindGlobal(VkCommandBuffer commandBuffer);

		bool DispatchEntity(VkCommandBuffer commandBuffer, GameEntity* entity);

		void Dispatch(VkCommandBuffer commandBuffer);

		inline VkPipeline GetPipeline() const { return m_pipeline; }
		inline const SPIRVComputeProgram* GetProgram() const { return m_program; }
		inline Material* GetMaterial() const { return m_material; }

	private:
		bool DispatchSlot(VkCommandBuffer commandBuffer, UInt32 slot);

		VkDevice m_device;
		VkPipeline m_pipeline;
		const SPIRVComputeProgram* m_program;
		Material* m_material;

		Ptr<VulkanMaterialResourceManager> m_resourceManager;
		std::vector<std::array<UInt32, 3>> m_threadGroupCounts;
	};
}
