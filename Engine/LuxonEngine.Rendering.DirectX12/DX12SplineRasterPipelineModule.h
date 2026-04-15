#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "Core/Vector3.h"
#include "Core/Vector2.h"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering{
	class SplineRenderer;
}

namespace QuantumEngine::Rendering::DX12 {
	struct SplineRendererData;

	namespace Compute {
		class HLSLComputeProgram;
	}

	namespace Rasterization {
		class DX12RasterizationMaterial;
	}

	struct SplineVertex {
		Vector3 position;
		Vector2 uv;
		Vector3 normal;
		Vector3 tangent;

		SplineVertex() : SplineVertex(Vector3(0.0f), Vector2(0.0f), Vector3(0.0f), Vector3(0.0f))
		{
		}

		SplineVertex(const Vector3& pos, const Vector2& tex, const Vector3& normal, const Vector3& tangent)
			:position(pos), uv(tex), normal(normal), tangent(tangent)
		{
		}
	};

	struct SplineParameters {
		Vector3 startPoint;
		float width;
		Vector3 midPoint;
		float length;
		Vector3 endPoint;
		float padding;
	};


	class DX12SplineRasterPipelineModule
	{
	public: 		
		DX12SplineRasterPipelineModule(const SplineRendererData& splineData, DXGI_FORMAT depthFormat, const ref<Compute::HLSLComputeProgram>& computeProgram);
		bool Initialize(const ComPtr<ID3D12Device10>& device);		
		void Render(ComPtr<ID3D12GraphicsCommandList7>& commandList, D3D12_GPU_DESCRIPTOR_HANDLE camHandle, D3D12_GPU_DESCRIPTOR_HANDLE lightHandle);
		UInt32 BindDescriptorToResources(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UInt32 offset);
	private:
		ref<SplineRenderer> m_splineRenderer;

		ComPtr<ID3D12Resource2> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_bufferView;
		ComPtr<ID3D12Device> m_device;

		ref<Rasterization::DX12RasterizationMaterial> m_material;
		D3D12_GPU_DESCRIPTOR_HANDLE m_transformHeapHandle;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipeline;
		DXGI_FORMAT m_depthFormat;

		ref<Compute::HLSLComputeProgram> m_computeProgram;
		ComPtr<ID3D12PipelineState> m_computePipeline;

		static D3D12_INPUT_ELEMENT_DESC s_inputElementDescs[4];
		static D3D12_INPUT_LAYOUT_DESC s_layoutDesc;

		UInt32 m_widthRootIndex;
		Float m_splineWidth;

		SplineParameters m_splineParams;
		UInt32 m_curveRootIndex;
		UInt32 m_vertexRootIndex;
		D3D12_GPU_DESCRIPTOR_HANDLE m_vertexBufferHandle;
	};
}
