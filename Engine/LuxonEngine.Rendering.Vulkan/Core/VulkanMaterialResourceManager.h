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

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanMaterialResourceManager {
	public:
		static constexpr UInt32 InvalidEntityIndex = ShaderVariableReflection::InvalidIndex;

		VulkanMaterialResourceManager(VkDevice device, Material* material, const ShaderVariableReflection& reflection,
			const std::vector<VkDescriptorSetLayout>& setLayouts, VkPipelineLayout pipelineLayout, VkShaderStageFlags stageFlags, VkPipelineBindPoint bindPoint);

		~VulkanMaterialResourceManager();

		VulkanMaterialResourceManager(const VulkanMaterialResourceManager&) = delete;
		VulkanMaterialResourceManager& operator=(const VulkanMaterialResourceManager&) = delete;

		bool Initialize(UInt32 entityCount = 0);

		void UpdateModifiedTextures();

		bool SetDescriptor(const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetDescriptor(const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return SetEntityConstantData(entity, name, &value, sizeof(T));
		}

		bool SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size);

		UInt32 RegisterEntity(GameEntity* entity);

		UInt32 FindEntity(GameEntity* entity) const;

		inline UInt32 GetEntitySlotCount() const { return (UInt32)m_entityDataList.size(); }

		inline GameEntity* GetEntity(UInt32 slot) const { return m_entityDataList[slot].entity; }

		void BindGlobal(VkCommandBuffer commandBuffer);

		void BindEntity(VkCommandBuffer commandBuffer, UInt32 slot);

	private:
		struct DescriptorData {
			const char* name;
			UInt32 binding;
			VkDescriptorType descriptorType;
			UInt32 descriptorCount = 1;
		};

		struct ConstantData {
			const char* name;
			UInt32 offset;
			UInt32 size;
			Byte* data = nullptr;
		};

		struct ConstantVariableLocation {
			UInt32 blockIndex;
			UInt32 offset;
			UInt32 size;
		};

		struct EntityDynamicData {
			GameEntity* entity = nullptr;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			std::vector<ConstantData> constants;
		};

		void InitializeDescriptorMapping();

		void InitializeConstantMapping();

		bool CreateDescriptorSet(const std::vector<VkDescriptorPoolSize>& poolSizes, UInt32 setIndex, VkDescriptorPool& pool, VkDescriptorSet& set) const;

		bool CreateEntityData(GameEntity* entity, EntityDynamicData& entityData) const;

		EntityDynamicData* GetOrRegisterEntity(GameEntity* entity);

		const DescriptorData* FindGlobalDescriptor(const char* name) const;

		const DescriptorData* FindEntityDescriptor(const char* name) const;

		bool WriteDescriptor(VkDescriptorSet set, const DescriptorData& descriptorData, const VkDescriptorBufferInfo* bufferInfo, const VkDescriptorImageInfo* imageInfo);

		void CopyMaterialTextures();

		void WriteTextureDescriptor(const MaterialTextureData& textureData);

		VkDevice m_device;
		Material* m_material;
		const ShaderVariableReflection& m_reflection;
		const std::vector<VkDescriptorSetLayout>& m_setLayouts;
		VkPipelineLayout m_pipelineLayout;
		VkShaderStageFlags m_stageFlags;
		VkPipelineBindPoint m_bindPoint;

		VkDescriptorPool m_materialDescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_materialDescriptorSet = VK_NULL_HANDLE;
		std::vector<VkDescriptorPoolSize> m_materialPoolSizes;
		std::vector<DescriptorData> m_materialDescriptors;
		std::vector<UInt32> m_textureDescriptorIndices;

		std::vector<VkDescriptorPoolSize> m_entityPoolSizes;
		std::vector<DescriptorData> m_entityDescriptors;
		std::map<std::string_view, UInt32> m_entityDescriptorLayout;

		std::vector<ConstantData> m_pushConstants;
		std::vector<ConstantData> m_entityConstantTemplate;
		std::map<std::string_view, ConstantVariableLocation> m_entityConstantLayout;

		std::vector<EntityDynamicData> m_entityDataList;
	};
}
