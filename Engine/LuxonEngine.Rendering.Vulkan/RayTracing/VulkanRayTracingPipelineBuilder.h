#pragma once
#include "vulkan-pch.h"
#include <map>
#include "Core/SPIRVReflection.h"

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::Vulkan::RayTracing {
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
		std::vector<std::set<Byte*>> datalocations;
	};

	struct EntitySBTConstant {
		Byte* location = nullptr;
		UInt32 size = 0;
	};

	struct EntitySBTData {
		UInt32 hitRecordIndex = VK_SHADER_UNUSED_KHR;
		std::map<std::string, EntitySBTConstant> constants;
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
		std::map<GameEntity*, EntitySBTData> entitySBTBlueprintMap;
		MaterialSBTData globalEntryIndex;
		UInt32 firstEntityHitRecord = 0;
		Byte* sbtMapped = nullptr;
		VkStridedDeviceAddressRegionKHR rayGenRegion;
		VkStridedDeviceAddressRegionKHR missRegion;
		VkStridedDeviceAddressRegionKHR hitRegion;
		VkStridedDeviceAddressRegionKHR callableRegion;
	};

	class VulkanRayTracingPipelineBuilder {
	public:
		static bool BuildRayTracingPipeline(const ref<SPIRVRayTracingProgram>& globalMaterial, const std::vector<ref<SPIRVRayTracingProgram>>& localPrograms, RayTracePipelineBuildResult& pipelineResult);
		static bool BuildRayTracingSBT(const ref<Material>& globalRTMaterial, const std::vector<ref<GameEntity>>& entities, const RayTracePipelineBuildResult& pipelineData, RayTraceSBTBuildResult& sbtResult);
	};
}