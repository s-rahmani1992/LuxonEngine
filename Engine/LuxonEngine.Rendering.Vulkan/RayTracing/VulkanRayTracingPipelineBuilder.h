#pragma once
#include "vulkan-pch.h"
#include <map>
#include "Core/SPIRVReflection.h"

namespace QuantumEngine::Rendering {
	class Material;
}

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class SPIRVRayTracingProgram;
	class SPIRVRayTracingProgramVariant;

	struct ShaderPipelineData {
		ref<SPIRVRayTracingProgramVariant> variant;
		UInt32 rayGenIndex = VK_SHADER_UNUSED_KHR;
		UInt32 missIndex = VK_SHADER_UNUSED_KHR;
		UINT32 hitGroupIndex = VK_SHADER_UNUSED_KHR;
	};

	struct MaterialSBTData {
		UInt32 missEntryIndex = VK_SHADER_UNUSED_KHR;
		UInt32 hitEntryIndex = VK_SHADER_UNUSED_KHR;
		std::vector<std::set<Byte*>> datalocations;
	};

	struct RayTracePipelineBuildResult {
		VkSampler sampler;
		VkPipelineLayout rtPipelineLayout;
		std::vector<VkDescriptorSetLayout> rtDescriptorLayouts;
		ShaderPipelineData globalProgramPipelineBlueprint;
		std::map<ref<SPIRVRayTracingProgram>, ShaderPipelineData> programPipelineBlueprintMap;
		VkPipeline rtPipeline;
		SPIRVReflection reflection;
		UInt32 groupCount;
	};

	struct RayTraceSBTBuildResult {
		VkBuffer sbtBuffer;
		VkDeviceMemory sbtMemory;
		std::map<ref<Material>, MaterialSBTData> materialSBTBlueprintMap;
		MaterialSBTData globalEntryIndex;
		VkStridedDeviceAddressRegionKHR rayGenRegion;
		VkStridedDeviceAddressRegionKHR missRegion;
		VkStridedDeviceAddressRegionKHR hitRegion;
		VkStridedDeviceAddressRegionKHR callableRegion;
	};

	class VulkanRayTracingPipelineBuilder {
	public:
		static bool BuildRayTracingPipeline(const ref<SPIRVRayTracingProgram>& globalMaterial, const std::vector<ref<SPIRVRayTracingProgram>>& localPrograms, RayTracePipelineBuildResult& pipelineResult);
		static bool BuildRayTracingSBT(const ref<Material>& globalRTMaterial, const std::vector<ref<Material>>& localMaterials, const RayTracePipelineBuildResult& pipelineData, RayTraceSBTBuildResult& sbtResult);
	};
}