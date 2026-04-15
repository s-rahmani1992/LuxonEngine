#include "vulkan-pch.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include "Core/VulkanDeviceManager.h"
#include "RayTracing/SPIRVRayTracingProgram.h"
#include "RayTracing/SPIRVRayTracingProgramVariant.h"
#include "Core/VulkanUtilities.h"
#include "Rendering/Material.h"
#include "Core/VulkanBufferFactory.h"

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineBuilder::BuildRayTracingPipeline(const ref<SPIRVRayTracingProgram>& globalRTProgram, const std::vector<ref<SPIRVRayTracingProgram>>& localPrograms, RayTracePipelineBuildResult& pipelineResult)
{
	auto device = VulkanDeviceManager::Instance()->GetGraphicDevice();

	VkSamplerCreateInfo samplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 0,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	auto result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &pipelineResult.sampler);
	
	if (result != VK_SUCCESS)
		return false;
	
	UInt32 bindingOffset = 0;
	pipelineResult.globalProgramPipelineBlueprint.variant = globalRTProgram->CreateVariantForRT(bindingOffset);

	pipelineResult.reflection.AddShaderReflection(pipelineResult.globalProgramPipelineBlueprint.variant->GetReflectionModule(), true);

	for (auto& localProgram : localPrograms) {

		auto it = pipelineResult.programPipelineBlueprintMap.emplace(localProgram, ShaderPipelineData{});

		if (it.second == false)
			continue;

		auto variantProgram = localProgram->CreateVariantForRT(bindingOffset);

		it.first->second.variant = variantProgram;
		pipelineResult.reflection.AddShaderReflection(variantProgram->GetReflectionModule(), true);
	}

	pipelineResult.rtDescriptorLayouts.resize(pipelineResult.reflection.GetDescriptorLayoutCount());
	pipelineResult.reflection.CreatePipelineLayout(device, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR
		, pipelineResult.sampler, &pipelineResult.rtPipelineLayout, pipelineResult.rtDescriptorLayouts.data());

	std::vector<VkPipelineShaderStageCreateInfo> stages;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

	VkRayTracingShaderGroupCreateInfoKHR missOrGenGroup{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = VK_SHADER_UNUSED_KHR,
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	};
	VkRayTracingShaderGroupCreateInfoKHR hitGroup{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
		.generalShader = VK_SHADER_UNUSED_KHR,
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	};

	UInt32 stageIndex = 0;
	UInt32 groupIndex = 0;
	auto rtGlobalStages = pipelineResult.globalProgramPipelineBlueprint.variant->GetStages();
	stages.insert(stages.end(), rtGlobalStages.begin(), rtGlobalStages.end());

	for (auto& stage : rtGlobalStages) {
		if (stage.stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR) {
			missOrGenGroup.generalShader = stageIndex;
			groups.push_back(missOrGenGroup);
			pipelineResult.globalProgramPipelineBlueprint.rayGenIndex = groupIndex;
			groupIndex++;
			stageIndex++;
			continue;
		}

		if (stage.stage == VK_SHADER_STAGE_MISS_BIT_KHR) {
			missOrGenGroup.generalShader = stageIndex;
			groups.push_back(missOrGenGroup);
			pipelineResult.globalProgramPipelineBlueprint.missIndex = groupIndex;
			groupIndex++;
			stageIndex++;
			continue;
		}

		if (stage.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) {
			hitGroup.closestHitShader = stageIndex;
			stageIndex++;
			continue;
		}

		if (stage.stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR) {
			hitGroup.intersectionShader = stageIndex;
			stageIndex++;
			continue;
		}

		if (stage.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR) {
			hitGroup.anyHitShader = stageIndex;
			stageIndex++;
			continue;
		}
	}

	if (hitGroup.closestHitShader != VK_SHADER_UNUSED_KHR
		|| hitGroup.intersectionShader != VK_SHADER_UNUSED_KHR
		|| hitGroup.anyHitShader != VK_SHADER_UNUSED_KHR)
	{
		groups.push_back(hitGroup);
		pipelineResult.globalProgramPipelineBlueprint.hitGroupIndex = groupIndex;
		groupIndex++;
	}

	for (auto& [program, shaderPipelineData] : pipelineResult.programPipelineBlueprintMap) {
		hitGroup.anyHitShader = hitGroup.closestHitShader = hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

		auto localStages = shaderPipelineData.variant->GetStages();
		stages.insert(stages.end(), localStages.begin(), localStages.end());

		for (auto& stage : localStages) {
			if (stage.stage == VK_SHADER_STAGE_MISS_BIT_KHR) {
				missOrGenGroup.generalShader = stageIndex;
				groups.push_back(missOrGenGroup);
				shaderPipelineData.missIndex = groupIndex;
				groupIndex++;
				stageIndex++;
				continue;
			}

			if (stage.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) {
				hitGroup.closestHitShader = stageIndex;
				stageIndex++;
				continue;
			}

			if (stage.stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR) {
				hitGroup.intersectionShader = stageIndex;
				stageIndex++;
				continue;
			}

			if (stage.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR) {
				hitGroup.anyHitShader = stageIndex;
				stageIndex++;
				continue;
			}
		}

		if (hitGroup.closestHitShader != VK_SHADER_UNUSED_KHR
			|| hitGroup.intersectionShader != VK_SHADER_UNUSED_KHR
			|| hitGroup.anyHitShader != VK_SHADER_UNUSED_KHR)
		{
			groups.push_back(hitGroup);
			shaderPipelineData.hitGroupIndex = groupIndex;
			groupIndex++;
		}
	}

	VkRayTracingPipelineCreateInfoKHR rtPipelineInfo{
	.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
	.stageCount = (UInt32)stages.size(),
	.pStages = stages.data(),
	.groupCount = (uint32_t)groups.size(),
	.pGroups = groups.data(),
	.maxPipelineRayRecursionDepth = 7,
	.layout = pipelineResult.rtPipelineLayout,
	};

	auto createRayTracingPipelinesPtr = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");

	createRayTracingPipelinesPtr(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtPipelineInfo, nullptr, &pipelineResult.rtPipeline);

	pipelineResult.groupCount = (uint32_t)groups.size();
	return true;
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineBuilder::BuildRayTracingSBT(const ref<Material>& globalRTMaterial, const std::vector<ref<Material>>& localMaterials, const RayTracePipelineBuildResult& pipelineData, RayTraceSBTBuildResult& sbtResult)
{
	auto device = VulkanDeviceManager::Instance()->GetGraphicDevice();

	auto globalRtProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(globalRTMaterial->GetProgram());

	auto rtProperties = VulkanDeviceManager::Instance()->GetRayTracingPipelineProperties();

	UInt32 dataHandleSize = pipelineData.groupCount * rtProperties->shaderGroupHandleSize;

	std::vector<UInt8> shaderHandleStorage(dataHandleSize);

	auto getShaderGroupHandlesPtr = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
	getShaderGroupHandlesPtr(device, pipelineData.rtPipeline, 0, pipelineData.groupCount, dataHandleSize, shaderHandleStorage.data());



	UInt32 missSBTCount = 1;
	sbtResult.globalEntryIndex.missEntryIndex = 0;
	UInt32 hitSBTCount = globalRtProgram->HasHitGroup() ? 1 : 0;
	sbtResult.globalEntryIndex.hitEntryIndex = 0;

	for (auto& material : localMaterials) {
		auto it = sbtResult.materialSBTBlueprintMap.emplace(material, MaterialSBTData{});
		
		if (it.second == false)
			continue;

		auto localProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(material->GetProgram());
		it.first->second.datalocations.resize(material->GetValueFields()->size());

		if (localProgram->HasMissStage()) {
			it.first->second.missEntryIndex = missSBTCount;
			missSBTCount++;
		}

		if (localProgram->HasHitGroup()) {
			it.first->second.hitEntryIndex = hitSBTCount;
			hitSBTCount++;
		}
	}

	UInt32 groupHandleSize = rtProperties->shaderGroupHandleSize;
	UInt32 groupHandleAlignment = rtProperties->shaderGroupHandleAlignment;
	UInt32 baseAlignment = rtProperties->shaderGroupBaseAlignment;

	UInt32 raygenSize = ALIGN(groupHandleSize + globalRtProgram->GetShaderRecordSize(), groupHandleAlignment);


	UInt32 missEntrySize = groupHandleSize + globalRtProgram->GetShaderRecordSize();
	UInt32 hitEntrySize = globalRtProgram->HasHitGroup() ? groupHandleSize + globalRtProgram->GetShaderRecordSize() : 0;

	for (auto& [material, matSBTData] : sbtResult.materialSBTBlueprintMap) {
		auto localProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(material->GetProgram());

		if (matSBTData.missEntryIndex != VK_SHADER_UNUSED_KHR) {
			UInt32 entrySize = groupHandleSize + localProgram->GetShaderRecordSize();
			if (entrySize > missEntrySize)
				missEntrySize = entrySize;
		}

		if (matSBTData.hitEntryIndex != VK_SHADER_UNUSED_KHR) {
			UInt32 entrySize = groupHandleSize + localProgram->GetShaderRecordSize();
			if (entrySize > hitEntrySize)
				hitEntrySize = entrySize;
		}
	}

	missEntrySize = ALIGN(missEntrySize, groupHandleAlignment);;
	UInt32 missSize = missSBTCount * missEntrySize;

	hitEntrySize = ALIGN(hitEntrySize, groupHandleAlignment);
	UInt32 hitSize = hitSBTCount * hitEntrySize;
	UInt32 callableSize = 0;  // No callable shaders for now

	// Ensure each region starts at a baseAlignment boundary
	UInt32 raygenOffset = 0;
	UInt32 missOffset = ALIGN(raygenSize, baseAlignment);
	UInt32 hitOffset = ALIGN(missOffset + missSize, baseAlignment);
	UInt32 callableOffset = ALIGN(hitOffset + hitSize, baseAlignment);

	auto SBTBufferSize = callableOffset + callableSize;

	VkMemoryAllocateFlagsInfo allocFlags{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	};

	auto bufferFactory = VulkanDeviceManager::Instance()->GetBufferFactory();

	bufferFactory->CreateBuffer(SBTBufferSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocFlags, &sbtResult.sbtBuffer, &sbtResult.sbtMemory);

	// map and copy handles at aligned offset
	
	void* mapped;
	vkMapMemory(device, sbtResult.sbtMemory, 0, VK_WHOLE_SIZE, 0, &mapped);

	UInt8* dst = (UInt8*)(mapped);

	auto copyToEntry = [&shaderHandleStorage, &groupHandleSize, &sbtResult](const ref<Material>& rtMaterial, UInt8* sectionStart, UInt32 groupIndex, UInt32 missEntryIndex) {
		std::memcpy(sectionStart, shaderHandleStorage.data() + groupHandleSize * groupIndex, groupHandleSize);
		auto program = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(rtMaterial->GetProgram());
		auto& shaderRecordVariables = program->GetShaderRecordVariables();
		auto materialFields = rtMaterial->GetValueFields();

		for (auto& recordVar : shaderRecordVariables) {
			if (recordVar.name == "_missIndex") {
				std::memcpy(sectionStart + groupHandleSize + recordVar.offset, &missEntryIndex, sizeof(UInt32));
				continue;
			}

			auto d = materialFields->find(recordVar.name);

			if (d != materialFields->end()) {				
				if(program->IsGlobal() == false)
					sbtResult.materialSBTBlueprintMap[rtMaterial].datalocations[d->second.fieldIndex].emplace(sectionStart + groupHandleSize + recordVar.offset);
				else
					std::memcpy(sectionStart + groupHandleSize + recordVar.offset, (*d).second.data, recordVar.size);
			}
		}
	};

	// Copy Ray Generation

	copyToEntry(globalRTMaterial, dst + raygenOffset, pipelineData.globalProgramPipelineBlueprint.rayGenIndex, sbtResult.globalEntryIndex.missEntryIndex);

	// Copy Miss Entries
	copyToEntry(globalRTMaterial, dst + missOffset + missEntrySize * sbtResult.globalEntryIndex.missEntryIndex, pipelineData.globalProgramPipelineBlueprint.missIndex, sbtResult.globalEntryIndex.missEntryIndex);

	for (auto& matPair : sbtResult.materialSBTBlueprintMap) {
		if (matPair.second.missEntryIndex == VK_SHADER_UNUSED_KHR)
			continue;

		auto localProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(matPair.first->GetProgram());
		copyToEntry(matPair.first, dst + missOffset + missEntrySize * matPair.second.missEntryIndex, pipelineData.programPipelineBlueprintMap.at(localProgram).missIndex, matPair.second.missEntryIndex);
	}

	// Copy Hitgroup

	if (globalRtProgram->HasHitGroup()) {
		copyToEntry(globalRTMaterial, dst + hitOffset + hitEntrySize * sbtResult.globalEntryIndex.hitEntryIndex, pipelineData.globalProgramPipelineBlueprint.hitGroupIndex, sbtResult.globalEntryIndex.missEntryIndex);
	}

	for (auto& [material, matSBTData] : sbtResult.materialSBTBlueprintMap) {
		if (matSBTData.hitEntryIndex == VK_SHADER_UNUSED_KHR)
			continue;

		auto localProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(material->GetProgram());
		copyToEntry(material, dst + hitOffset + hitEntrySize * matSBTData.hitEntryIndex, pipelineData.programPipelineBlueprintMap.at(localProgram).hitGroupIndex, matSBTData.missEntryIndex);
	}

	vkUnmapMemory(device,sbtResult.sbtMemory);

	VkBufferDeviceAddressInfo addrInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = sbtResult.sbtBuffer,
	};

	VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(device, &addrInfo);

	sbtResult.rayGenRegion.deviceAddress = sbtAddress + raygenOffset;
	sbtResult.rayGenRegion.stride = raygenSize;
	sbtResult.rayGenRegion.size = raygenSize;
	
	sbtResult.missRegion.deviceAddress = sbtAddress + missOffset;
	sbtResult.missRegion.stride = missEntrySize;
	sbtResult.missRegion.size = missSize;

	sbtResult.hitRegion.deviceAddress = sbtAddress + hitOffset;
	sbtResult.hitRegion.stride = hitEntrySize;
	sbtResult.hitRegion.size = hitSize;

	int h = 0;
	return true;
}
