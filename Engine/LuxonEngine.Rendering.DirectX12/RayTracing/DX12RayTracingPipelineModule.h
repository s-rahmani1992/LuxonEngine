#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <memory>
#include <vector>
#include <map>

namespace QuantumEngine {
	class GameEntity;
	class Transform;
	class GPUAssetManager;
	class ShaderProgram;
	struct SceneLightData;
	class Camera;

	namespace Rendering {
		class Material;
	}
}

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	struct DX12RayTracingGPUData;
	//class HLSLShaderProgram;
}

namespace QuantumEngine::Rendering::DX12::RayTracing {
	class RTAccelarationStructure;
	struct EntityBLASDesc;
	class HLSLRayTracingProgram;
	class DX12RayTracingMaterial;

	class DX12RayTracingPipelineModule
	{
	public:
		bool Initialize(const ComPtr<ID3D12GraphicsCommandList7>& commandList, const std::vector<DX12RayTracingGPUData>& entities, UInt32 width, UInt32 height, const ref<Material> globalRTMaterial, const ComPtr<ID3D12Resource2>& camera, const ComPtr<ID3D12Resource2>& light);
		void RenderCommand(ComPtr<ID3D12GraphicsCommandList7>& commandList, const ref<Camera>& camera);
		ComPtr<ID3D12Resource2> GetOutputBuffer() const { return m_outputBuffer; }
		ComPtr<ID3D12DescriptorHeap> GetOutputHeap() const { return m_outputHeap; }
		ref<RayTracing::DX12RayTracingMaterial> GetMaterialInterface() { return m_globalRTMaterial; }
	private:
		bool InitializeShaderTable(const std::vector<DX12RayTracingGPUData>& entities, UInt32 heapOffset);
		bool InitializePipeline();

		ComPtr<ID3D12Device10> m_device;
		ComPtr<ID3D12Resource2> m_outputBuffer;
		ComPtr<ID3D12DescriptorHeap> m_outputHeap;

		ref<RayTracing::RTAccelarationStructure> m_TLASController;
		ComPtr<ID3D12StateObject> m_rtStateObject;
		ComPtr<ID3D12Resource2> m_shaderTableBuffer;

		ref<Material> m_rtglobalMaterial;
		ref<RayTracing::HLSLRayTracingProgram> m_globalRTProgram;
		ref<RayTracing::DX12RayTracingMaterial> m_globalRTMaterial;
		ComPtr<ID3D12DescriptorHeap> m_rtHeap;
		std::map<ref<Material>, ref<RayTracing::DX12RayTracingMaterial>> dxMaterialMap;
		D3D12_DISPATCH_RAYS_DESC m_raytraceDesc;
	};
}
