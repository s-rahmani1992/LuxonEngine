#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"
#include "Rendering/ShaderReflection.h"
#include <map>
#include <string_view>
#include <vector>

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
	struct MaterialTextureData;
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
			return SetEntityConstantData(entity, name, &value, sizeof(T));
		}

		inline VkPipeline GetPipeline() const { return m_pipeline; }
		inline const SPIRVMeshProgram* GetProgram() const { return m_meshProgram; }
		inline Material* GetMaterial() const { return m_material; }

	private:
		// a resource variable of a descriptor set of the program
		struct DescriptorData {
			const char* name; // points into the reflection of the program
			UInt32 binding;
			VkDescriptorType descriptorType;
			UInt32 descriptorCount = 1;
		};

		// a push constant block of the variable reflection
		struct ConstantData {
			const char* name; // first variable of the block, points into the reflection of the program
			UInt32 offset; // byte offset in the push constant range
			UInt32 size; // size in bytes
			Byte* data = nullptr; // material data for material blocks, allocated per entity for dynamic blocks
		};

		// location of a variable of the dynamic push constant blocks
		struct ConstantVariableLocation {
			UInt32 blockIndex; // index in EntityDynamicData::constants
			UInt32 offset; // byte offset in the data of the block
			UInt32 size; // size in bytes
		};

		struct EntityDynamicData {
			GameEntity* entity = nullptr;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			std::vector<ConstantData> constants;
			UInt32 threadGroupCount[3] = { 0, 0, 0 }; // group count of the first stage (task or mesh), the entity is not drawn while it is zero
		};

		void InitializeDescriptorMapping();

		void InitializeConstantMapping();

		bool SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size);

		bool CreateDescriptorSet(const std::vector<VkDescriptorPoolSize>& poolSizes, UInt32 setIndex, VkDescriptorPool& pool, VkDescriptorSet& set) const;

		bool CreateEntityData(GameEntity* entity, EntityDynamicData& entityData) const;

		EntityDynamicData* GetOrRegisterEntity(GameEntity* entity);

		const DescriptorData* FindGlobalDescriptor(const char* name) const;

		const DescriptorData* FindEntityDescriptor(const char* name) const;

		bool WriteDescriptor(VkDescriptorSet set, const DescriptorData& descriptorData, const VkDescriptorBufferInfo* bufferInfo, const VkDescriptorImageInfo* imageInfo);

		void CopyMaterialTextures();

		void WriteTextureDescriptor(const MaterialTextureData& textureData);

		VkDevice m_device;
		VkPipeline m_pipeline;
		PFN_vkCmdDrawMeshTasksEXT m_cmdDrawMeshTasks = nullptr; // extension function, loaded from the device
		const SPIRVMeshProgram* m_meshProgram;
		Material* m_material;

		// material and global variables, set = 0
		VkDescriptorPool m_materialDescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_materialDescriptorSet = VK_NULL_HANDLE;
		std::vector<VkDescriptorPoolSize> m_materialPoolSizes;
		std::vector<DescriptorData> m_materialDescriptors;
		std::vector<UInt32> m_textureDescriptorIndices; // texture field index of the material -> index in m_materialDescriptors

		// per entity variables, set = 1. every entity has its own pool and set with this layout
		std::vector<VkDescriptorPoolSize> m_entityPoolSizes;
		std::vector<DescriptorData> m_entityDescriptors;
		std::map<std::string_view, UInt32> m_entityDescriptorLayout; // name -> index in m_entityDescriptors

		// push constants. all the blocks share the single push constant range of the program
		std::vector<ConstantData> m_pushConstants; // material push constant blocks
		std::vector<ConstantData> m_entityConstantTemplate; // dynamic push constant blocks, copied and allocated for every entity
		std::map<std::string_view, ConstantVariableLocation> m_entityConstantLayout; // dynamic constant variables of an entity

		std::vector<EntityDynamicData> m_entityDataList;
	};
}
