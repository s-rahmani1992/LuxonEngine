#include "pch.h"
#include "RTStateObjectBuilder.h"
#include "HLSLRayTracingProgram.h"

namespace LuxonEngine::Rendering::DX12::RayTracing {
	ComPtr<ID3D12StateObject> RTStateObjectBuilder::Create(ID3D12Device10* device, const RayTracingPipelineProperties& properties,
		HLSLRayTracingProgram* globalProgram, const std::vector<HLSLRayTracingProgram*>& localPrograms, std::string& error)
	{
		if (globalProgram == nullptr || globalProgram->GetRootSignature() == nullptr) {
			error = "The global ray tracing program has no root signature";
			return nullptr;
		}

		struct LocalProgramData {
			std::vector<LPCWSTR> exports;
			D3D12_LOCAL_ROOT_SIGNATURE localRootSignature;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rootSignatureAssociation;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociation;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION pipelineConfigAssociation;
		};

		// the sub objects point to each other, so the vectors must not grow
		std::vector<D3D12_STATE_SUBOBJECT> subobjects;
		subobjects.reserve(8 + 6 * localPrograms.size());

		std::vector<LocalProgramData> localDatas;
		localDatas.reserve(localPrograms.size());

		// Global program
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
			.pDesc = globalProgram->GetDXIL(),
			});

		D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature{
			.pGlobalRootSignature = globalProgram->GetRootSignature().Get(),
		};

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
			.pDesc = &globalRootSignature,
			});

		D3D12_STATE_SUBOBJECT* globalRootSignaturePtr = &subobjects.back();

		D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{
			.MaxPayloadSizeInBytes = properties.maxPayloadSize,
			.MaxAttributeSizeInBytes = properties.maxAttributeSize,
		};

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
			.pDesc = &shaderConfig,
			});

		D3D12_STATE_SUBOBJECT* shaderConfigPtr = &subobjects.back();

		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{
			.MaxTraceRecursionDepth = properties.maxRecursionDepth,
		};

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
			.pDesc = &pipelineConfig,
			});

		D3D12_STATE_SUBOBJECT* pipelineConfigPtr = &subobjects.back();

		auto globalExports = globalProgram->GetExportNames();

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION globalRootAssociation{
			.pSubobjectToAssociate = globalRootSignaturePtr,
			.NumExports = (UINT)globalExports.size(),
			.pExports = globalExports.data(),
		};

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION globalShaderConfigAssociation = globalRootAssociation;
		globalShaderConfigAssociation.pSubobjectToAssociate = shaderConfigPtr;

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION globalPipelineConfigAssociation = globalRootAssociation;
		globalPipelineConfigAssociation.pSubobjectToAssociate = pipelineConfigPtr;

		for (auto* association : { &globalRootAssociation, &globalShaderConfigAssociation, &globalPipelineConfigAssociation }) {
			subobjects.push_back(D3D12_STATE_SUBOBJECT{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
				.pDesc = association,
				});
		}

		if (globalProgram->HasHitGroup()) {
			subobjects.push_back(D3D12_STATE_SUBOBJECT{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
				.pDesc = globalProgram->GetHitGroupDesc(),
				});
		}

		// Local programs
		for (auto& program : localPrograms) {
			if (program == nullptr || program->GetRootSignature() == nullptr) {
				error = "A local ray tracing program has no root signature";
				return nullptr;
			}

			auto& localData = localDatas.emplace_back();
			localData.exports = program->GetExportNames();

			// an association without exports would apply to all the exports
			if (localData.exports.empty())
				continue;

			subobjects.push_back(D3D12_STATE_SUBOBJECT{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
				.pDesc = program->GetDXIL(),
				});

			if (program->HasHitGroup()) {
				subobjects.push_back(D3D12_STATE_SUBOBJECT{
					.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
					.pDesc = program->GetHitGroupDesc(),
					});
			}

			localData.localRootSignature.pLocalRootSignature = program->GetRootSignature().Get();

			subobjects.push_back(D3D12_STATE_SUBOBJECT{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
				.pDesc = &localData.localRootSignature,
				});

			D3D12_STATE_SUBOBJECT* localRootSignaturePtr = &subobjects.back();

			localData.rootSignatureAssociation = D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{
				.pSubobjectToAssociate = localRootSignaturePtr,
				.NumExports = (UINT)localData.exports.size(),
				.pExports = localData.exports.data(),
			};

			localData.shaderConfigAssociation = localData.rootSignatureAssociation;
			localData.shaderConfigAssociation.pSubobjectToAssociate = shaderConfigPtr;

			localData.pipelineConfigAssociation = localData.rootSignatureAssociation;
			localData.pipelineConfigAssociation.pSubobjectToAssociate = pipelineConfigPtr;

			for (auto* association : { &localData.rootSignatureAssociation, &localData.shaderConfigAssociation, &localData.pipelineConfigAssociation }) {
				subobjects.push_back(D3D12_STATE_SUBOBJECT{
					.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
					.pDesc = association,
					});
			}
		}

		D3D12_STATE_OBJECT_DESC stateObjectDesc{
			.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.NumSubobjects = (UINT)subobjects.size(),
			.pSubobjects = subobjects.data(),
		};

		ComPtr<ID3D12StateObject> stateObject;

		if (FAILED(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject)))) {
			error = "Failed to create the ray tracing state object";
			return nullptr;
		}

		return stateObject;
	}
}
