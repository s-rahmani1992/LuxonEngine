#pragma once
#include "vulkan-pch.h"
#include "Core/SPIRVReflection.h"
#include <map>

namespace QuantumEngine {
	class GameEntity;
}

namespace QuantumEngine::Rendering {
	class Material;
}

namespace QuantumEngine::Rendering::Vulkan {
	struct VKEntityGPUData;
	class VulkanBufferFactory;
	class VulkanMeshController;
}

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class VulkanBLAS;
	class SPIRVRayTracingProgram;
	class SPIRVRayTracingProgramVariant;

	class VulkanRayTracingPipelineModule {
	public:
		VulkanRayTracingPipelineModule();
		~VulkanRayTracingPipelineModule();

		bool Initialize(std::vector<ref<GameEntity>>& entities, const ref<Material> rtMaterial, VkBuffer camBuffer, VkBuffer lightBuffer, VkBuffer transformBuffer, const VkExtent2D& extent);
		void RenderCommand(VkCommandBuffer commandBuffer);
		void UpdateTLAS(VkCommandBuffer commandBuffer);
		void SetImage(const std::string& name, const VkImageView imageView);
	private:
		void WriteBuffers(const std::string name, const VkBuffer buffer);
		void WriteArrayBuffer(const std::string name, const std::vector<VkBuffer>& buffers);
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
		VkExtent2D m_extent;
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

		std::vector<VkBuffer> m_indexStorageBuffers;
		std::vector<VkBuffer> m_vertexStorageBuffers;

		std::map<ref<SPIRVRayTracingProgramVariant>, ProgramResourceData> m_resourceMaps;
	};
}