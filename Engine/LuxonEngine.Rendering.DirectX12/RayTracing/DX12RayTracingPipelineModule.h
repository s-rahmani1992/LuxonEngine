#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12PipelineFactory.h"
#include "DX12RayTracingResourceTable.h"
#include "RTSceneAccelerationStructure.h"
#include <memory>
#include <vector>

using namespace Microsoft::WRL;

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEngine::Rendering::DX12 {
	class DX12MeshController;
}

namespace LuxonEngine::Rendering::DX12::RayTracing {
	class HLSLRayTracingProgram;


	class DX12RayTracingPipelineModule {
	public:
		DX12RayTracingPipelineModule(ID3D12Device10* device, const RayTracingPipelineProperties& properties);

		bool Initialize(Material* globalMaterial, std::vector<RayTracingEntityDesc> initialEntities = {});

		bool SetDescriptor(const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		bool SetEntityDescriptor(GameEntity* entity, const char* name, D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle);

		void SetDimensions(UInt32 width, UInt32 height);

		bool Dispatch(ID3D12GraphicsCommandList7* commandList);

	private:
		bool BuildInitialStructures();

		ComPtr<ID3D12Device10> m_device;
		RayTracingPipelineProperties m_properties;
		Material* m_globalMaterial;
		HLSLRayTracingProgram* m_globalProgram;
		std::vector<HLSLRayTracingProgram*> m_localPrograms; // distinct programs of the materials of the entities
		std::vector<RayTracingEntityDesc> m_initialEntities;

		D3D12_DISPATCH_RAYS_DESC m_dispatchDesc = {};

		ComPtr<ID3D12StateObject> m_stateObject;
		Ptr<RTSceneAccelerationStructure> m_accelerationStructure;
		Ptr<DX12RayTracingResourceTable> m_resourceTable;
	};
}
