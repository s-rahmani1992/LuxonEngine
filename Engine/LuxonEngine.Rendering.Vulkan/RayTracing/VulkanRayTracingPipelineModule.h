#pragma once
#include "vulkan-pch.h"
#include "Core/SPIRVReflection.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include <map>

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::Vulkan {
	struct VKEntityGPUData;
	class VulkanBufferFactory;
	class VulkanMeshController;
}

namespace LuxonEngine::Rendering::Vulkan::RayTracing {
	class VulkanBLAS;
	class SPIRVRayTracingProgram;
	class SPIRVRayTracingProgramVariant;

	class VulkanRayTracingPipelineModule {
	public:
		VulkanRayTracingPipelineModule();
		~VulkanRayTracingPipelineModule();

		bool Initialize(const std::vector<ref<GameEntity>>& entities, const ref<Material>& rtMaterial);
		void RenderCommand(VkCommandBuffer commandBuffer);
		void UpdateTLAS(VkCommandBuffer commandBuffer);
		bool SetBuffer(const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
		bool SetImage(const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL);
		bool SetEntityBuffer(GameEntity* entity, const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
		bool SetEntityImage(GameEntity* entity, const char* name, VkImageView imageView, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL);
		void SetExtent(const VkExtent2D& extent);
		bool SetEntityConstantData(GameEntity* entity, const char* name, const void* data, UInt32 size);

		template<typename T>
		bool SetEntityConstant(GameEntity* entity, const char* name, const T& value) {
			return SetEntityConstantData(entity, name, &value, sizeof(T));
		}
	private:
		bool WriteDescriptor(const DescriptableBufferData& descriptorData, UInt32 arrayElement, const VkDescriptorBufferInfo* bufferInfo, const VkDescriptorImageInfo* imageInfo);
		const DescriptableBufferData* FindEntityDescriptor(GameEntity* entity, const char* name, UInt32& instanceIndex);
		struct VKEntityGPUData {
		public:
			ref<GameEntity> gameEntity;
			UInt32 index;
		};

		struct MaterialTextureFieldData {
			UInt32 binding;
			UInt32 set;
		};

		struct MaterialResourceDatas {
			UInt32 textureArrayIndex;
			std::vector<std::set<Byte*>> datalocations;
		};

		struct ProgramResourceData {
			std::map<ref<Material>, MaterialResourceDatas> materialIndexMap;
			std::vector<MaterialTextureFieldData> images;
		};

		struct DescriptorData {
			UInt32 setIndex;
			UInt32 binding;
			VkDescriptorType descriptorType;
		};

		struct MaterialSBTData {
			UInt32 missEntryIndex = VK_SHADER_UNUSED_KHR;
			UInt32 hitEntryIndex = VK_SHADER_UNUSED_KHR;
			std::vector<std::set<Byte*>> datalocations;
		};

		VkDevice m_device;
		VkDescriptorPool m_descriptorPool;
		ref<VulkanBufferFactory> m_bufferFactory;

		std::vector<VKEntityGPUData> m_entities;

		std::map<ref<VulkanMeshController>, ref<VulkanBLAS>> m_blasMap;
		VkQueue m_graphicsQueue;
		VkExtent2D m_extent = {};
		VkAccelerationStructureKHR m_tlas;
		VkDeviceAddress m_tlasAddress;
		VkBuffer m_tlasBuffer;
		VkDeviceMemory m_tlasMemory;

		std::vector<VkAccelerationStructureInstanceKHR> m_vkBLASInstances;
		VkBuffer m_instanceBuffer;
		VkDeviceMemory m_instanceMemory;
		VkDeviceAddress m_instanceBufferAddress;
		VkBuffer m_scratchBuffer;
		VkDeviceMemory m_scratchMemory;
		VkDeviceAddress m_baseScratchAddress;

		VkAccelerationStructureGeometryKHR m_asGeom;
		VkAccelerationStructureBuildGeometryInfoKHR m_TLASBuildInfo;

		std::map<GameEntity*, EntitySBTData> m_entitySBTData;

		VkBuffer m_SBT;
		VkDeviceMemory m_SBTMemory;

		ref<SPIRVRayTracingProgram> m_globalRtProgram;
		VkPipeline m_rtPipeline;

		std::vector<VkDescriptorSet> m_descriptorSets;		

		VkStridedDeviceAddressRegionKHR m_rayGenRegion;
		VkStridedDeviceAddressRegionKHR m_missRegion;
		VkStridedDeviceAddressRegionKHR m_hitRegion;
		VkStridedDeviceAddressRegionKHR m_callableRegion = {0, 0, 0};

		std::map<ref<SPIRVRayTracingProgram>, ref<SPIRVRayTracingProgramVariant>> m_programVariantMap;
		VkPipelineLayout m_rtPipelineLayout;
		std::vector<VkDescriptorSetLayout> m_descriptorLayouts;
		VkSampler m_sampler;
		ref<Material> m_globalMaterial;
		SPIRVReflection m_reflection;

		std::map<ref<SPIRVRayTracingProgramVariant>, ProgramResourceData> m_resourceMaps;
	};
}