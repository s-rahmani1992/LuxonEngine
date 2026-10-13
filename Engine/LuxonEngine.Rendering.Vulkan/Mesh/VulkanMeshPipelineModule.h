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

namespace LuxonEngine::Rendering::Vulkan::MeshShading {
	class SPIRVMeshProgram;

	class VulkanMeshPipelineModule {
	public:
		VulkanMeshPipelineModule(VkDevice device, VkPipeline pipeline, const SPIRVMeshProgram* program, Material* material);

		~VulkanMeshPipelineModule();

		VulkanMeshPipelineModule(const VulkanMeshPipelineModule&) = delete;
		VulkanMeshPipelineModule& operator=(const VulkanMeshPipelineModule&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		void Dispatch(VkCommandBuffer commandBuffer);

		bool SetEntityThreadGroupCount(GameEntity* entity, UInt32 x, UInt32 y = 1, UInt32 z = 1);

		bool SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return m_resourceManager->SetEntityConstant(entity, name, value);
		}

		inline VkPipeline GetPipeline() const { return m_pipeline; }
		inline const SPIRVMeshProgram* GetProgram() const { return m_meshProgram; }
		inline Material* GetMaterial() const { return m_material; }

	private:
		struct ThreadGroupCount {
			UInt32 x = 0;
			UInt32 y = 0;
			UInt32 z = 0;
		};

		VkDevice m_device;
		VkPipeline m_pipeline;
		PFN_vkCmdDrawMeshTasksEXT m_cmdDrawMeshTasks = nullptr;
		const SPIRVMeshProgram* m_meshProgram;
		Material* m_material;

		Ptr<VulkanMaterialResourceManager> m_resourceManager;
		std::vector<ThreadGroupCount> m_entityThreadGroupCounts;
	};
}
