#include "vulkan-pch.h"
#include "VulkanRayTracingPipelineModule.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanMeshController.h"
#include "Core/VulkanGraphicContext.h"
#include "Core/GameEntity.h"
#include "Core/Transform.h"
#include "Rendering/RayTracingComponent.h"
#include "Rendering/GPUMeshController.h"
#include "Core/Mesh.h"
#include "Core/VulkanBufferFactory.h"
#include "RayTracing/VulkanBLAS.h"
#include "Core/VulkanUtilities.h"
#include "SPIRVRayTracingProgram.h"
#include "SPIRVRayTracingProgramVariant.h"
#include "Rendering/Material.h"
#include "VulkanRayTracingPipelineBuilder.h"
#include <algorithm>
#include <iterator>
#include "Core/Texture2D.h"
#include "Core/VulkanTexture2DController.h"

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::VulkanRayTracingPipelineModule()
	: m_device(VulkanDeviceManager::Instance()->GetGraphicDevice()),
	m_bufferFactory(VulkanDeviceManager::Instance()->GetBufferFactory()),
	m_graphicsQueue(VulkanDeviceManager::Instance()->GetGraphicsQueue())
{
}

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::~VulkanRayTracingPipelineModule()
{
	vkDestroyBuffer(m_device, m_scratchBuffer, nullptr);
	vkFreeMemory(m_device, m_scratchMemory, nullptr);
	vkDestroyBuffer(m_device, m_instanceBuffer, nullptr);
	vkFreeMemory(m_device, m_instanceMemory, nullptr);
	auto vkDestroyAccelerationStructurePtr = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR");
	vkDestroyAccelerationStructurePtr(m_device, m_tlas, nullptr);
	vkDestroyBuffer(m_device, m_tlasBuffer, nullptr);
	vkFreeMemory(m_device, m_tlasMemory, nullptr);

	vkDestroyBuffer(m_device, m_SBT, nullptr);
	vkFreeMemory(m_device, m_SBTMemory, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::Initialize(std::vector<ref<GameEntity>>& entities, const ref<Material> rtMaterial, VkBuffer camBuffer, VkBuffer lightBuffer, VkBuffer transformBuffer, const VkExtent2D& extent)
{
	m_extent = extent;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = VulkanDeviceManager::Instance()->GetGraphicsQueueFamilyIndex(),
	};

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		return false;

	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS)
		return false;

	// Create Pipeline
	m_globalMaterial = rtMaterial;
	m_globalRtProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(rtMaterial->GetProgram());

	RayTracePipelineBuildResult pipelineResult;
	std::vector<ref<SPIRVRayTracingProgram>> localPrograms;

	std::transform(entities.begin(), entities.end(), std::back_inserter(localPrograms),
		[](const ref<GameEntity> entity)
		{
			auto rtComponent = entity->GetRayTracingComponent();

			if (rtComponent == nullptr)
				return ref<SPIRVRayTracingProgram>{nullptr};

			return std::dynamic_pointer_cast<SPIRVRayTracingProgram>(rtComponent->GetRTMaterial()->GetProgram()); 
		});

	VulkanRayTracingPipelineBuilder::BuildRayTracingPipeline(m_globalRtProgram, localPrograms, pipelineResult);

	for (auto& matPair : pipelineResult.programPipelineBlueprintMap)
		m_programVariantMap.emplace(matPair.first, matPair.second.variant);

	m_rtPipelineLayout = pipelineResult.rtPipelineLayout;
	m_descriptorLayouts.insert(m_descriptorLayouts.end(), pipelineResult.rtDescriptorLayouts.begin(), pipelineResult.rtDescriptorLayouts.end());
	m_rtPipeline = pipelineResult.rtPipeline;
	m_sampler = pipelineResult.sampler;
	m_reflection = pipelineResult.reflection;

	// Create SBT
	RayTraceSBTBuildResult sbtBuildResult;
	std::vector<ref<Material>> localRTMaterials;

	std::transform(entities.begin(), entities.end(), std::back_inserter(localRTMaterials),
		[](const ref<GameEntity> entity)
		{
			auto rtComponent = entity->GetRayTracingComponent();

			if (rtComponent == nullptr)
				return ref<Material>{nullptr};

			return rtComponent->GetRTMaterial();
		});
	VulkanRayTracingPipelineBuilder::BuildRayTracingSBT(m_globalMaterial, localRTMaterials, pipelineResult, sbtBuildResult);

	m_SBT = sbtBuildResult.sbtBuffer;
	m_SBTMemory = sbtBuildResult.sbtMemory;
	m_rayGenRegion = sbtBuildResult.rayGenRegion;
	m_missRegion = sbtBuildResult.missRegion;
	m_hitRegion = sbtBuildResult.hitRegion;


	for (auto& program : pipelineResult.programPipelineBlueprintMap) {
		m_resourceMaps.emplace(program.second.variant, ProgramResourceData{});
	}

	for (auto& mat : sbtBuildResult.materialSBTBlueprintMap) {
		auto localProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(mat.first->GetProgram());
		auto& matResourceData = m_resourceMaps[pipelineResult.programPipelineBlueprintMap.at(localProgram).variant];
		matResourceData.materialIndexMap.emplace(mat.first, MaterialResourceDatas{
			.textureArrayIndex = (UInt32)matResourceData.materialIndexMap.size(),
			.datalocations = mat.second.datalocations,
			});
	}

	////// Create Acceleration Structures

	// Create BLASs
	std::map<ref<VulkanMeshController>, VulkanBLASBuildInfo> meshbuildInfos;

	UInt32 index = 0;
	for(auto& entityData : entities) {
		auto rtComponent = entityData->GetRayTracingComponent();

		if(rtComponent == nullptr)
			continue;

		m_entities.push_back({ entityData, index });
		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(rtComponent->GetMesh()->GetGPUHandle());
		
		auto it = meshbuildInfos.emplace(meshController, VulkanBLASBuildInfo{});
		if (it.second) {
			meshController->GetBLASBuildInfo(&((*it.first).second));
		}
	}

	UInt32 scratchOffsetAlignment = VulkanDeviceManager::Instance()->GetAccelerationStructureProperties()->minAccelerationStructureScratchOffsetAlignment;
	UInt32 scratchBufferSize = 0;

	for(auto& [meshController, buildInfo] : meshbuildInfos) {
		scratchBufferSize += ALIGN(buildInfo.sizeInfo.buildScratchSize, scratchOffsetAlignment);
	}

    VkMemoryAllocateFlagsInfo allocFlags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
    };

	m_bufferFactory->CreateBuffer(scratchBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &m_scratchBuffer, &m_scratchMemory);

    VkBufferDeviceAddressInfo scratchAddrInfo{};
    scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchAddrInfo.buffer = m_scratchBuffer;

    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &scratchAddrInfo);
	m_baseScratchAddress = scratchAddress;
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	for(auto& [meshController, buildInfo] : meshbuildInfos) {
		auto blas = std::make_shared<VulkanBLAS>();
		buildInfo.buildInfo.scratchData.deviceAddress = scratchAddress;
		blas->CreateCommand(commandBuffer, &buildInfo);
		m_blasMap.emplace(meshController, blas);
		scratchAddress += ALIGN(buildInfo.sizeInfo.buildScratchSize, scratchOffsetAlignment);
	}

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	// Create TLAS

	m_vkBLASInstances.reserve(m_entities.size());
	Matrix4 m;

	for(auto& entityData : m_entities) {
		auto rtComponent = entityData.gameEntity->GetRayTracingComponent();
		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(rtComponent->GetMesh()->GetGPUHandle());
		auto& blas = m_blasMap[meshController];
		auto program = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(rtComponent->GetRTMaterial()->GetProgram());
		auto& blasInstance = m_vkBLASInstances.emplace_back(VkAccelerationStructureInstanceKHR{});

		VulkanBLASBuildInfo buildInfo = meshbuildInfos[meshController];
		UInt32 primitiveCount = buildInfo.primitiveCount;
		m = entityData.gameEntity->GetTransform()->Matrix();
		std::memcpy(&blasInstance.transform, &m, 12 * sizeof(Float));
		blasInstance.instanceCustomIndex = m_resourceMaps[pipelineResult.programPipelineBlueprintMap[program].variant].materialIndexMap[rtComponent->GetRTMaterial()].textureArrayIndex;
		blasInstance.mask = 0xFF;
		blasInstance.instanceShaderBindingTableRecordOffset = sbtBuildResult.materialSBTBlueprintMap[rtComponent->GetRTMaterial()].hitEntryIndex;
		blasInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		blasInstance.accelerationStructureReference = blas->GetBLASAddress();
	}

	VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_vkBLASInstances.size();
	m_bufferFactory->CreateBuffer(instanceBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocFlags, &m_instanceBuffer, &m_instanceMemory);

	void* data = nullptr;
	vkMapMemory(m_device, m_instanceMemory, 0, instanceBufferSize, 0, &data);
	memcpy(data, m_vkBLASInstances.data(), instanceBufferSize);
	vkUnmapMemory(m_device, m_instanceMemory);

	VkBufferDeviceAddressInfo instanceBufferAddressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = m_instanceBuffer,
	};

	m_instanceBufferAddress = vkGetBufferDeviceAddress(m_device, &instanceBufferAddressInfo);

	VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
	instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instancesData.arrayOfPointers = VK_FALSE;
	instancesData.data.deviceAddress = m_instanceBufferAddress;

	m_asGeom.pNext = nullptr;
	m_asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	m_asGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	m_asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	m_asGeom.geometry.instances = instancesData;

	UInt32 primitiveCount = static_cast<UInt32>(m_entities.size());

	// 3. Build sizes
	VkAccelerationStructureBuildGeometryInfoKHR TLASBuildInfo{
		VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	TLASBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	TLASBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
						  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	TLASBuildInfo.geometryCount = 1;
	TLASBuildInfo.pGeometries = &m_asGeom;

	VkAccelerationStructureBuildSizesInfoKHR TLASsizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	auto getRTASGetSizePtr = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR");

	getRTASGetSizePtr(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &TLASBuildInfo, &primitiveCount, &TLASsizeInfo);

	if(TLASsizeInfo.buildScratchSize > scratchBufferSize) {
		// Recreate scratch buffer
		vkDestroyBuffer(m_device, m_scratchBuffer, nullptr);
		vkFreeMemory(m_device, m_scratchMemory, nullptr);
		scratchBufferSize = TLASsizeInfo.buildScratchSize;
		m_bufferFactory->CreateBuffer(scratchBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &m_scratchBuffer, &m_scratchMemory);
		VkBufferDeviceAddressInfo scratchAddrInfo{};
		scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchAddrInfo.buffer = m_scratchBuffer;
		m_baseScratchAddress = vkGetBufferDeviceAddress(m_device, &scratchAddrInfo);
	}

	// 4. Create TLAS buffer + acceleration structure
	m_bufferFactory->CreateBuffer(TLASsizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &m_tlasBuffer, &m_tlasMemory);

	VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	asCreateInfo.buffer = m_tlasBuffer;
	asCreateInfo.size = TLASsizeInfo.accelerationStructureSize;
	asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	auto createAccelerationStructurePtr = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR");

	createAccelerationStructurePtr(m_device, &asCreateInfo, nullptr, &m_tlas);

	VkAccelerationStructureBuildGeometryInfoKHR buildCmdInfo = TLASBuildInfo;
	buildCmdInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildCmdInfo.dstAccelerationStructure = m_tlas;
	buildCmdInfo.scratchData.deviceAddress = m_baseScratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
	rangeInfo.primitiveCount = primitiveCount;
	rangeInfo.primitiveOffset = 0;
	rangeInfo.firstVertex = 0;
	rangeInfo.transformOffset = 0;

	const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
	auto buildAccelerationStructurePtr = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR");

	vkResetCommandBuffer(commandBuffer, 0);
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	buildAccelerationStructurePtr(commandBuffer, 1, &buildCmdInfo, &pRangeInfo);
	vkEndCommandBuffer(commandBuffer);

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	// Material Descriptors
	auto& descriptors = m_reflection.GetDescriptors();
	
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(20);
	UInt32 setcount = m_reflection.GetDescriptorLayoutCount();

	for (auto& descriptor : descriptors) {
		auto it = std::find_if(poolSizes.begin(), poolSizes.end(), [descriptor](const VkDescriptorPoolSize& poolSize) {
			return descriptor.descriptorType == poolSize.type;
			});

		UInt32 newCount = descriptor.isDynamicArray ? 200 : 1; //TODO fix hard coded size for dynamic arrays

		if (it != poolSizes.end())
			(*it).descriptorCount += newCount;
		else
			poolSizes.push_back(VkDescriptorPoolSize{
			.type = descriptor.descriptorType,
			.descriptorCount = newCount,
				});
	}

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = 1,
		});

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = setcount,
		.poolSizeCount = (UInt32)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return false;

	m_descriptorSets.resize(m_descriptorLayouts.size());

	VkDescriptorSetAllocateInfo descSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = nullptr,
	};

	for (UInt32 i = 0; i < m_descriptorLayouts.size(); i++) {
		descSetAlloc.pSetLayouts = m_descriptorLayouts.data() + i;

		if (vkAllocateDescriptorSets(m_device, &descSetAlloc, m_descriptorSets.data() + i) != VK_SUCCESS)
			return false;
	}

	WriteBuffers(HLSL_CAMERA_DATA_NAME, camBuffer);
	WriteBuffers(HLSL_LIGHT_DATA_NAME, lightBuffer);
	WriteBuffers(HLSL_TRANSFORM_ARRAY, transformBuffer);

	auto descriptorData = m_reflection.GetDescriptorData(HLSL_RT_TLAS_SCENE_NAME);
	
	if (descriptorData != nullptr) {
		VkWriteDescriptorSetAccelerationStructureKHR asInfo{};
		asInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		asInfo.accelerationStructureCount = 1;
		asInfo.pAccelerationStructures = &m_tlas; // VkAccelerationStructureKHR

		VkWriteDescriptorSet writeAS{};
		writeAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeAS.dstSet = m_descriptorSets[descriptorData->data.set];
		writeAS.dstBinding = descriptorData->data.binding;
		writeAS.descriptorCount = 1;
		writeAS.descriptorType = descriptorData->descriptorType;
		writeAS.pNext = &asInfo;

		vkUpdateDescriptorSets(m_device, 1, &writeAS, 0, nullptr);
	}

	m_vertexStorageBuffers.reserve(m_entities.size());
	m_indexStorageBuffers.reserve(m_entities.size());

	for (auto& entityData : m_entities) {
		auto rtComponent = entityData.gameEntity->GetRayTracingComponent();
		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(rtComponent->GetMesh()->GetGPUHandle());
		
		m_vertexStorageBuffers.push_back(meshController->CreateVertexStorageBuffer());
		m_indexStorageBuffers.push_back(meshController->CreateIndexStorageBuffer());
	}

	WriteArrayBuffer(HLSL_RT_VERTEX_BUFFER_ARRAY, m_vertexStorageBuffers);
	WriteArrayBuffer(HLSL_RT_INDEX_BUFFER_ARRAY, m_indexStorageBuffers);

	for (auto& [variantProgram, matResourceData] : m_resourceMaps) {
		auto& material = matResourceData.materialIndexMap.begin()->first;

		auto& textureFields = *(material->GetTextureFields());
		matResourceData.images.resize(textureFields.size());

		for (auto& [name, MatTextureData] : textureFields) {
			auto& x = matResourceData.images[MatTextureData.fieldIndex];
			variantProgram->GetBindingAndSet(name + "Array", &x.binding, &x.set);
		}
	}

	vkDestroyCommandPool(m_device, commandPool, nullptr);

	return true;
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::RenderCommand(VkCommandBuffer commandBuffer)
{
	for (auto& [variantProgram, matResourceData]  : m_resourceMaps) {
		for (auto& [material, index] : matResourceData.materialIndexMap) {
			auto& innerTextureFields = material->GetModifiedTextures();

			if (innerTextureFields.size() == 0)
				continue;

			for (auto& modifiedTextureField : innerTextureFields) {
				auto& n = matResourceData.images[modifiedTextureField->fieldIndex];
				auto textureController = std::dynamic_pointer_cast<VulkanTexture2DController>(modifiedTextureField->texture->GetGPUHandle());

				VkDescriptorImageInfo info{};
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				info.imageView = textureController->GetImageView();
				info.sampler = VK_NULL_HANDLE;

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = m_descriptorSets[n.set];
				write.dstBinding = n.binding;              // matches HLSL binding
				write.dstArrayElement = index.textureArrayIndex;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.descriptorCount = 1;
				write.pImageInfo = &info;

				vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
			}

			material->ClearTextures();
		}

		for (auto& [material, matResourceData] : matResourceData.materialIndexMap) {
			auto& innerValueFields = material->GetModifiedValues();

			if (innerValueFields.size() == 0)
				continue;

			for (auto& modifiedValueField : innerValueFields) {
				auto& locations = matResourceData.datalocations[modifiedValueField->fieldIndex];
				
				for (auto& location : locations)
					std::memcpy(location, modifiedValueField->data, modifiedValueField->size);

			}

			material->ClearModifiedValues();
		}
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

	int shaderFlag = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 0, (UInt32)m_descriptorSets.size(), m_descriptorSets.data(), 0, nullptr);

	auto vkCmdTraceRaysPtr = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR");

	vkCmdTraceRaysPtr(commandBuffer, &m_rayGenRegion, &m_missRegion, &m_hitRegion, &m_callableRegion, m_extent.width, m_extent.height, 1);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::UpdateTLAS(VkCommandBuffer commandBuffer)
{
	Matrix4 m;

	for (UInt32 i = 0; i < m_entities.size(); i++) {
		m = m_entities[i].gameEntity->GetTransform()->Matrix();
		auto& blasInstance = m_vkBLASInstances[i];
		std::memcpy(&blasInstance.transform, &m, 12 * sizeof(Float));
	}

	void* data = nullptr;
	VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_vkBLASInstances.size();
	vkMapMemory(m_device, m_instanceMemory, 0, VK_WHOLE_SIZE, 0, &data);
	memcpy(data, m_vkBLASInstances.data(), instanceBufferSize);
	vkUnmapMemory(m_device, m_instanceMemory);

	VkAccelerationStructureBuildGeometryInfoKHR buildCmdInfo{
		VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildCmdInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildCmdInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	buildCmdInfo.geometryCount = 1;
	buildCmdInfo.pGeometries = &m_asGeom;

	buildCmdInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	buildCmdInfo.srcAccelerationStructure = m_tlas;
	buildCmdInfo.dstAccelerationStructure = m_tlas;
	buildCmdInfo.scratchData.deviceAddress = m_baseScratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
	rangeInfo.primitiveCount = m_entities.size();
	rangeInfo.primitiveOffset = 0;
	rangeInfo.firstVertex = 0;
	rangeInfo.transformOffset = 0;

	const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
	auto buildAccelerationStructurePtr = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR");

	buildAccelerationStructurePtr(commandBuffer, 1, &buildCmdInfo, &pRangeInfo);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::SetImage(const std::string& name, const VkImageView imageView)
{
	auto descriptorData = m_reflection.GetDescriptorData(name);

	if (descriptorData != nullptr) {
		VkDescriptorImageInfo imgDesc{};
		imgDesc.imageView = imageView;
		imgDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_descriptorSets[descriptorData->data.set];
		write.dstBinding = descriptorData->data.binding;
		write.descriptorType = descriptorData->descriptorType;
		write.descriptorCount = 1;
		write.pImageInfo = &imgDesc;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	}
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::WriteBuffers(const std::string name, const VkBuffer buffer)
{
	auto descriptorData = m_reflection.GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	VkDescriptorBufferInfo descBufferInfo{
		.buffer = buffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};

	VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptorSets[descriptorData->data.set],
		.dstBinding = descriptorData->data.binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorData->descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = &descBufferInfo,
		.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::WriteArrayBuffer(const std::string name, const std::vector<VkBuffer>& buffers)
{
	auto descriptorData = m_reflection.GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	std::vector<VkDescriptorBufferInfo> bufferInfos;
	bufferInfos.reserve(buffers.size());

	for (auto& buffer : buffers) {
		bufferInfos.push_back(VkDescriptorBufferInfo{
			.buffer = buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
			});
	}

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptorSets[descriptorData->data.set];
	write.dstBinding = descriptorData->data.binding;
	write.descriptorType = descriptorData->descriptorType;
	write.descriptorCount = (UInt32)(bufferInfos.size()); // Current dynamic size
	write.pBufferInfo = bufferInfos.data();

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
}
