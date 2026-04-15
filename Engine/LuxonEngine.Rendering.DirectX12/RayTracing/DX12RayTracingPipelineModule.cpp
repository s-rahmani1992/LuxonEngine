#include "pch.h"
#include "Dx12RayTracingPipelineModule.h"
#include "RayTracing/RTAccelarationStructure.h"
#include "DX12GraphicContext.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "DX12Utilities.h"
#include <set>
#include "HLSLShaderProgram.h"
#include "HLSLShader.h"
#include "DX12MeshController.h"
#include "Rendering/RayTracingComponent.h"
#include "RayTracing/HLSLRayTracingProgram.h"
#include <Rendering/Material.h>
#include "RayTracing/DX12RayTracingMaterial.h"

bool QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingPipelineModule::Initialize(const ComPtr<ID3D12GraphicsCommandList7>& commandList, const std::vector<DX12RayTracingGPUData>& entities, UInt32 width, UInt32 height, const ref<Material> globalRTMaterial, const ComPtr<ID3D12Resource2>& camera, const ComPtr<ID3D12Resource2>& light)
{
	m_raytraceDesc.Width = width;
	m_raytraceDesc.Height = height;
	m_raytraceDesc.Depth = 1;

	//m_rtMaterial = rtMaterial;
	commandList->GetDevice(IID_PPV_ARGS(&m_device));

	m_rtglobalMaterial = globalRTMaterial;
	m_globalRTProgram = std::dynamic_pointer_cast<RayTracing::HLSLRayTracingProgram>(globalRTMaterial->GetProgram());
	m_globalRTMaterial = std::make_shared<DX12::RayTracing::DX12RayTracingMaterial>(globalRTMaterial, m_globalRTProgram);
	// Create ResourceHeap and material allocations

	UInt32 globalDescriptorCount = 4 + m_globalRTMaterial->GetNonGlobalResourceCounts(); // (Camera + Light + TLAS + output) + the rest of global material resources

	// global + entity(transform + vertex + index) + global material
	UInt32 rtHeapsize = globalDescriptorCount;
	
	for (auto& entity : entities) {
		auto& rtMaterial = entity.material;

		auto meterialIt = dxMaterialMap.find(rtMaterial);

		if(dxMaterialMap.find(rtMaterial) != dxMaterialMap.end()) {
			rtHeapsize += (meterialIt->second->GetNonGlobalResourceCounts() - rtMaterial->GetTextureFields()->size());
			continue;
		}

		auto rtProgram = std::dynamic_pointer_cast<RayTracing::HLSLRayTracingProgram>(rtMaterial->GetProgram());
		auto dxMaterial = std::make_shared<DX12::RayTracing::DX12RayTracingMaterial>(rtMaterial, rtProgram);
		dxMaterialMap.emplace(rtMaterial, dxMaterial);
		rtHeapsize += dxMaterial->GetNonGlobalResourceCounts();
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = rtHeapsize,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};

	m_device->CreateDescriptorHeap(&rtHeapDesc, IID_PPV_ARGS(&m_rtHeap));

	////// Populate heap
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto heapStartHandle = m_rtHeap->GetCPUDescriptorHandleForHeapStart();
	auto gpuStartHandle = m_rtHeap->GetGPUDescriptorHandleForHeapStart();

	// Camera
	D3D12_RESOURCE_DESC rDesc = camera->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cameraViewDesc;
	cameraViewDesc.BufferLocation = camera->GetGPUVirtualAddress();
	cameraViewDesc.SizeInBytes = rDesc.Width;
	m_device->CreateConstantBufferView(&cameraViewDesc, heapStartHandle);
	auto cameraHandle = gpuStartHandle;

	heapStartHandle.ptr += incSize;
	gpuStartHandle.ptr += incSize;

	// Light
	rDesc = light->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC lightViewDesc;
	lightViewDesc.BufferLocation = light->GetGPUVirtualAddress();
	lightViewDesc.SizeInBytes = rDesc.Width;
	m_device->CreateConstantBufferView(&lightViewDesc, heapStartHandle);
	auto lightHandle = gpuStartHandle;

	heapStartHandle.ptr += incSize;
	gpuStartHandle.ptr += incSize;

	// Initialize TLAS
	m_TLASController = std::make_shared<RayTracing::RTAccelarationStructure>();
	std::vector<RayTracing::EntityBLASDesc> rtEntities;

	UInt32 hitIndex = m_globalRTProgram->HasHitGroup() ? 1 : 0;

	for (auto& entity : entities) {
		rtEntities.push_back(RayTracing::EntityBLASDesc{
			.hitGroupIndex = hitIndex,
			.mesh = entity.meshController,
			.transform = entity.transform,
			});

		hitIndex++;
	}

	if (m_TLASController->Initialize(commandList, rtEntities) == false)
		return false;

	D3D12_SHADER_RESOURCE_VIEW_DESC tlasSRVDesc = {};
	tlasSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	tlasSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tlasSRVDesc.RaytracingAccelerationStructure.Location = m_TLASController->GetResource()->GetGPUVirtualAddress();
	m_device->CreateShaderResourceView(nullptr, &tlasSRVDesc, heapStartHandle);
	auto tlasHandle = gpuStartHandle;

	heapStartHandle.ptr += incSize;
	gpuStartHandle.ptr += incSize;

	// Ray Tracing Output Buffer
	// Create the output resource. The dimensions should match the swap-chain
	D3D12_RESOURCE_DESC outputDesc = {};
	outputDesc.DepthOrArraySize = 1;
	outputDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	outputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	outputDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	outputDesc.Width = width;
	outputDesc.Height = height;
	outputDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	outputDesc.MipLevels = 1;
	outputDesc.SampleDesc.Count = 1;

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&outputDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_outputBuffer)
	))) {
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavOutputDesc = {};
	uavOutputDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavOutputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateUnorderedAccessView(m_outputBuffer.Get(), nullptr, &uavOutputDesc, heapStartHandle);
	auto outputHandle = gpuStartHandle;
	
	heapStartHandle.ptr += incSize;
	gpuStartHandle.ptr += incSize;

	m_globalRTMaterial->SetDescriptorHandle(HLSL_CAMERA_DATA_NAME, cameraHandle);
	m_globalRTMaterial->SetDescriptorHandle(HLSL_LIGHT_DATA_NAME, lightHandle);
	m_globalRTMaterial->SetDescriptorHandle(HLSL_RT_TLAS_SCENE_NAME, tlasHandle);
	m_globalRTMaterial->SetDescriptorHandle(HLSL_RT_OUTPUT_TEXTURE_NAME, outputHandle);

	UInt32 offset = (heapStartHandle.ptr - m_rtHeap->GetCPUDescriptorHandleForHeapStart().ptr) / incSize;
	offset += m_globalRTMaterial->LinkMaterialToDescriptorHeap(m_rtHeap, offset);

	for(auto& rtMaterialPair : dxMaterialMap) {
		auto& rtMaterial = rtMaterialPair.second;
		rtMaterial->SetDescriptorHandle(HLSL_CAMERA_DATA_NAME, cameraHandle);
		rtMaterial->SetDescriptorHandle(HLSL_LIGHT_DATA_NAME, lightHandle);
		rtMaterial->SetDescriptorHandle(HLSL_RT_TLAS_SCENE_NAME, tlasHandle);
		rtMaterial->SetDescriptorHandle(HLSL_RT_OUTPUT_TEXTURE_NAME, outputHandle);
		offset += rtMaterial->LinkUserMaterials(m_rtHeap, offset);
	}

	InitializePipeline();
	InitializeShaderTable(entities, offset);

	return true;
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingPipelineModule::RenderCommand(ComPtr<ID3D12GraphicsCommandList7>& commandList, const ref<Camera>& camera)
{
	for (auto& mat : dxMaterialMap)
		mat.second->UpdateModifiedParameters();

	Matrix4 v = camera->ViewMatrix();
	m_TLASController->UpdateTransforms(commandList, v);

	commandList->SetComputeRootSignature((m_globalRTProgram->GetRootSignature()).Get());
	commandList->SetPipelineState1(m_rtStateObject.Get());

	commandList->SetDescriptorHeaps(1, m_rtHeap.GetAddressOf());
	m_globalRTMaterial->BindParameters(commandList);
	// Run Ray Tracing Pipeline

	D3D12_RESOURCE_BARRIER outputBeginRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_outputBuffer.Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COMMON,
		.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		},
	};
	commandList->ResourceBarrier(1, &outputBeginRTBarrier);

	commandList->DispatchRays(&m_raytraceDesc);

	D3D12_RESOURCE_BARRIER outputEndRTBarrier = outputBeginRTBarrier;
	outputEndRTBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	outputEndRTBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	commandList->ResourceBarrier(1, &outputEndRTBarrier);
}

bool QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingPipelineModule::InitializeShaderTable(const std::vector<DX12RayTracingGPUData>& entities, UInt32 heapOffset)
{
	UInt32 rayGenResordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	rayGenResordSize = SBT_SHADER_RECORD_ALIGHT(rayGenResordSize);
	UInt32 rayGenSectionSize = SBT_SECTION_ALIGHT(rayGenResordSize);

	UInt32 rtMissCount = 0;
	UInt32 missResordSize = 0;

	if (m_globalRTProgram->HasMissStage()) {
		m_globalRTMaterial->GetProgram();

		missResordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_globalRTProgram->GetReflectionData()->GetTotalVariableSize();
		missResordSize = SBT_SHADER_RECORD_ALIGHT(missResordSize);
		m_globalRTMaterial->SetInternalConstantValue("_missIndex", rtMissCount);
		rtMissCount++;
	}

	for (auto& rtPair : dxMaterialMap) {
		if (rtPair.second->GetProgram()->HasMissStage() == false)
			continue;

		UInt32 missSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + rtPair.second->GetProgram()->GetReflectionData()->GetTotalVariableSize();
		missSize = SBT_SHADER_RECORD_ALIGHT(missSize);
		rtPair.second->SetInternalConstantValue("_missIndex", rtMissCount);

		if (missSize > missResordSize)
			missResordSize = missSize;

		rtMissCount++;
	}

	UInt32 missSectionSize = SBT_SECTION_ALIGHT(rtMissCount * missResordSize);

	while (missSectionSize % missResordSize)
		missSectionSize += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	UInt32 hitRecordSize = 0;
	UInt32 rtEntityCount = 0;

	if (m_globalRTProgram->HasHitGroup()) {
		hitRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_globalRTProgram->GetReflectionData()->GetTotalVariableSize();
		hitRecordSize = SBT_SHADER_RECORD_ALIGHT(hitRecordSize);
		rtEntityCount++;
	}

	for (auto& entity : entities) {
		auto& rtMaterial = dxMaterialMap[entity.material];
		auto localRTProgram = rtMaterial->GetProgram();
		UInt32 hitGroupSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + localRTProgram->GetReflectionData()->GetTotalVariableSize();
		hitGroupSize = SBT_SHADER_RECORD_ALIGHT(hitGroupSize);

		if (hitGroupSize > hitRecordSize)
			hitRecordSize = hitGroupSize;

		rtEntityCount++;
	}

	UInt32 hitSectionSize = SBT_SECTION_ALIGHT(rtEntityCount * hitRecordSize);

	while (hitSectionSize % hitRecordSize > 0)
		hitSectionSize += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	ComPtr<ID3D12StateObjectProperties> rtProperties;
	if (FAILED(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&rtProperties))))
		return false;

	D3D12_RESOURCE_DESC shaderTableBufferDesc
	{
	shaderTableBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	shaderTableBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	shaderTableBufferDesc.Width = rayGenSectionSize + missSectionSize + hitSectionSize,
	shaderTableBufferDesc.Height = 1,
	shaderTableBufferDesc.DepthOrArraySize = 1,
	shaderTableBufferDesc.MipLevels = 1,
	shaderTableBufferDesc.Format = DXGI_FORMAT_UNKNOWN,
	shaderTableBufferDesc.SampleDesc.Count = 1,
	shaderTableBufferDesc.SampleDesc.Quality = 0,
	shaderTableBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	shaderTableBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
		&shaderTableBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_shaderTableBuffer))))
		return false;

	// Fill Shader Table

	Byte* pData;
	m_shaderTableBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));

	// Entry 0 - ray-gen program ID and descriptor data
	std::memcpy(pData, rtProperties->GetShaderIdentifier(m_globalRTProgram->GetRayGenExportName().c_str()),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 1 - primary ray miss
	pData += rayGenSectionSize;
	Byte* pMissData = pData;
	std::memcpy(pMissData, rtProperties->GetShaderIdentifier(m_globalRTProgram->GetMissExportName().c_str()),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	m_globalRTMaterial->CopyVariableData(pMissData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	pMissData += missResordSize;

	for (auto& rtPair : dxMaterialMap) {
		if (rtPair.second->GetProgram()->HasMissStage() == false)
			continue;

		std::memcpy(pMissData, rtProperties->GetShaderIdentifier(rtPair.second->GetProgram()->GetMissExportName().c_str()),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		rtPair.second->CopyVariableData(pMissData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		pMissData += missResordSize;
	}

	// Entry 2 - hit program
	pData += missSectionSize;
	Byte* pHitData = pData;

	if (m_globalRTProgram->HasHitGroup()) {
		std::memcpy(pHitData, rtProperties->GetShaderIdentifier(m_globalRTProgram->GetHitGroupDesc()->HitGroupExport),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		m_globalRTMaterial->CopyVariableData(pHitData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		pHitData += hitRecordSize;
	}

	UInt32 offset = heapOffset;

	for (auto& entity : entities) {
		auto rtMaterial = dxMaterialMap[entity.material];
		auto k = rtProperties->GetShaderIdentifier(rtMaterial->GetProgram()->GetHitGroupDesc()->HitGroupExport);
		std::memcpy(pHitData, rtProperties->GetShaderIdentifier(rtMaterial->GetProgram()->GetHitGroupDesc()->HitGroupExport),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		offset += rtMaterial->LinkInternalMaterials(m_rtHeap, offset);

		D3D12_CONSTANT_BUFFER_VIEW_DESC transformcbvDesc = {};
		transformcbvDesc.BufferLocation = entity.transformResource->GetGPUVirtualAddress();
		transformcbvDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(TransformGPU));		
		rtMaterial->SetCBV(HLSL_OBJECT_TRANSFORM_DATA_NAME, transformcbvDesc);

		auto vertexSRV = entity.meshController->GetVertexSRVDesc();
		rtMaterial->SetSRV(HLSL_VERTEX_SRV_NAME, entity.meshController->GetVertexResource(), vertexSRV);

		auto indexSRV = entity.meshController->GetIndexSRVDesc();
		rtMaterial->SetSRV(HLSL_INDEX_SRV_NAME, entity.meshController->GetIndexResource(), indexSRV);

		rtMaterial->CopyVariableData(pHitData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		pHitData += hitRecordSize;
	}

	// Unmap
	m_shaderTableBuffer->Unmap(0, nullptr);

	D3D12_GPU_VIRTUAL_ADDRESS startAddress = m_shaderTableBuffer->GetGPUVirtualAddress();
	// RayGen is the first entry in the shader-table
	m_raytraceDesc.RayGenerationShaderRecord.StartAddress = startAddress;
	m_raytraceDesc.RayGenerationShaderRecord.SizeInBytes = rayGenSectionSize;

	// Miss is the second entry in the shader-table
	startAddress += rayGenSectionSize;
	m_raytraceDesc.MissShaderTable.StartAddress = startAddress;
	m_raytraceDesc.MissShaderTable.StrideInBytes = missResordSize;
	m_raytraceDesc.MissShaderTable.SizeInBytes = missSectionSize; // Only a s single miss-entry 

	// Hit is the fourth entry in the shader-table
	startAddress += missSectionSize;
	m_raytraceDesc.HitGroupTable.StartAddress = startAddress;
	m_raytraceDesc.HitGroupTable.StrideInBytes = hitRecordSize;
	m_raytraceDesc.HitGroupTable.SizeInBytes = hitSectionSize;

	m_raytraceDesc.CallableShaderTable.StartAddress = 0;
	m_raytraceDesc.CallableShaderTable.StrideInBytes = 0;
	m_raytraceDesc.CallableShaderTable.SizeInBytes = 0;

	return true;
}

bool QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingPipelineModule::InitializePipeline()
{
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(20 + 20 * dxMaterialMap.size());

	// Create DXIL Global Sub Object
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
		.pDesc = m_globalRTProgram->GetDXIL(),
		});

	// Global Root Signature
	D3D12_GLOBAL_ROOT_SIGNATURE globalRt{
		.pGlobalRootSignature = m_globalRTProgram->GetRootSignature().Get(),
	};

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
		.pDesc = &globalRt,
		});

	D3D12_STATE_SUBOBJECT* rootPtr = subobjects.data() + subobjects.size() - 1;

	// Shader Config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = 6 * sizeof(Float);
	shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(Float);

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
		.pDesc = &shaderConfig,
		});

	D3D12_STATE_SUBOBJECT* shaderConfigPtr = subobjects.data() + subobjects.size() - 1;

	// Pipeline Config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = 7;

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
		.pDesc = &pipelineConfig,
		});

	D3D12_STATE_SUBOBJECT* pipelineConfigPtr = subobjects.data() + subobjects.size() - 1;

	auto globalExports = m_globalRTProgram->GetExportNames();

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rootExportDesc;
	rootExportDesc.pSubobjectToAssociate = rootPtr;
	rootExportDesc.NumExports = globalExports.size();
	rootExportDesc.pExports = globalExports.data();
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
		.pDesc = &rootExportDesc,
		});

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigExportDesc;
	shaderConfigExportDesc.pSubobjectToAssociate = shaderConfigPtr;
	shaderConfigExportDesc.NumExports = globalExports.size();
	shaderConfigExportDesc.pExports = globalExports.data();
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
		.pDesc = &shaderConfigExportDesc,
		});

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION pipelineConfigExportDesc;
	pipelineConfigExportDesc.pSubobjectToAssociate = pipelineConfigPtr;
	pipelineConfigExportDesc.NumExports = globalExports.size();
	pipelineConfigExportDesc.pExports = globalExports.data();
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
		.pDesc = &pipelineConfigExportDesc,
		});

	if (m_globalRTProgram->HasHitGroup()) {
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
			.pDesc = m_globalRTProgram->GetHitGroupDesc(),
			});
	}

	//Create Local RootSignature state objects for each program

	struct LocalRTData {
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hitExportDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigExportDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION pipelineConfigExportDesc;
		D3D12_LOCAL_ROOT_SIGNATURE localRootSignature; // TODO check later if this can be removed
		std::vector<LPCWSTR> hitExportPtr;
	};

	std::set<ref<RayTracing::HLSLRayTracingProgram>> programs;
	std::vector<LocalRTData> entityRTDatas;
	entityRTDatas.reserve(dxMaterialMap.size() + 1); // Prevent vector from reallocation. So the pointers are always valid

	for (auto& matPair : dxMaterialMap) {
		auto& rtMaterial = matPair.second;
		entityRTDatas.push_back(LocalRTData{});
		LocalRTData& rtRef = entityRTDatas.back();
		ref<RayTracing::HLSLRayTracingProgram> localProgram = rtMaterial->GetProgram();

		if (programs.emplace(localProgram).second == false) 
			continue;

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
			.pDesc = localProgram->GetDXIL(),
			});

		//// Create HitGroup for local material
		rtRef.hitExportPtr = localProgram->GetExportNames();

		if (localProgram->HasHitGroup()) {
			subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
			.pDesc = localProgram->GetHitGroupDesc(),
			});
		}

		rtRef.localRootSignature.pLocalRootSignature = localProgram->GetRootSignature().Get();

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
		.pDesc = &(rtRef.localRootSignature),
			});

		D3D12_STATE_SUBOBJECT* localRootPtr = subobjects.data() + subobjects.size() - 1;

		rtRef.hitExportDesc.pSubobjectToAssociate = localRootPtr;
		rtRef.hitExportDesc.NumExports = rtRef.hitExportPtr.size();
		rtRef.hitExportDesc.pExports = rtRef.hitExportPtr.data();
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.hitExportDesc,
			});

		rtRef.shaderConfigExportDesc.pSubobjectToAssociate = shaderConfigPtr;
		rtRef.shaderConfigExportDesc.NumExports = rtRef.hitExportPtr.size();
		rtRef.shaderConfigExportDesc.pExports = rtRef.hitExportPtr.data();
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.shaderConfigExportDesc,
			});

		rtRef.pipelineConfigExportDesc.pSubobjectToAssociate = pipelineConfigPtr;
		rtRef.pipelineConfigExportDesc.NumExports = rtRef.hitExportPtr.size();
		rtRef.pipelineConfigExportDesc.pExports = rtRef.hitExportPtr.data();
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.pipelineConfigExportDesc,
			});
	}

	// Create the State Object
	D3D12_STATE_OBJECT_DESC stateObjectDesc;
	stateObjectDesc.NumSubobjects = subobjects.size();
	stateObjectDesc.pSubobjects = subobjects.data();
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	if (FAILED(m_device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_rtStateObject)))) {
		return false;
	}

	return true;
}
