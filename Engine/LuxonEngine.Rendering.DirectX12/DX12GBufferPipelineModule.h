#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "Core/Vector2UInt.h"
#include <vector>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering {
	class GBufferRTReflectionRenderer;
}

namespace QuantumEngine::Rendering::DX12 {
	class DX12MeshController;
	class HLSLMaterial;
	struct EntityGBufferData;

	namespace Rasterization {
		class HLSLRasterizationProgram;
	}

	class DX12GBufferPipelineModule
	{
	public:
		bool Initialize(const ComPtr<ID3D12Device10>& device, const Vector2UInt& size, const ref<Rasterization::HLSLRasterizationProgram>& gBufferProgram);
		void RenderCommand(ComPtr<ID3D12GraphicsCommandList7>& commandList, D3D12_GPU_DESCRIPTOR_HANDLE camHandle);
		void PrepareEntities(const std::vector<EntityGBufferData>& entities);
		ComPtr<ID3D12DescriptorHeap> GetPositionHeap() { return m_positionHeap; }
		ComPtr<ID3D12DescriptorHeap> GetNormalHeap() { return m_normalHeap; }
		ComPtr<ID3D12DescriptorHeap> GetMaskHeap() { return m_maskHeap; }
	private:
		struct BufferEntityData{
			ref<DX12MeshController> meshController;
			D3D12_GPU_DESCRIPTOR_HANDLE transformHandle;
		};;

		bool CreatePipelineState(const ComPtr<ID3D12Device10>& device);
		DXGI_FORMAT m_gPositionFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		DXGI_FORMAT m_gNormalFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
		DXGI_FORMAT m_gMaskFormat = DXGI_FORMAT_R8_UINT;
		ComPtr<ID3D12Resource2> m_gPositionBuffer;
		ComPtr<ID3D12Resource2> m_gNormalBuffer;
		ComPtr<ID3D12Resource2> m_gMaskBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE m_gPositionHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE m_gNormalHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE m_gMaskHandle;

		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;

		ComPtr<ID3D12Resource2> m_depthBuffer;
		ComPtr<ID3D12DescriptorHeap> m_depthHeap;
		ComPtr<ID3D12DescriptorHeap> m_positionHeap;
		ComPtr<ID3D12DescriptorHeap> m_normalHeap;
		ComPtr<ID3D12DescriptorHeap> m_maskHeap;

		UInt32 m_cameraRootIndex;
		UInt32 m_transformRootIndex;
		ref<Rasterization::HLSLRasterizationProgram> m_program;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipeline;

		std::vector<BufferEntityData> m_entities;

		Vector2UInt m_size;
	};
}
