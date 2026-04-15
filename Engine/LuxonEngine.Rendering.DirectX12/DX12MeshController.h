#pragma once
#include "BasicTypes.h"
#include "Rendering/GPUMeshController.h"

using namespace Microsoft::WRL;

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering::DX12 {
	class DX12MeshController : public GPUMeshController
	{
	public:
		DX12MeshController(const ref<Mesh>& mesh);
		inline static D3D12_INPUT_LAYOUT_DESC* GetLayoutDesc() { return &s_layoutDesc; }
		inline D3D12_VERTEX_BUFFER_VIEW* GetVertexView() { return &m_bufferView; }
		inline D3D12_INDEX_BUFFER_VIEW* GetIndexView() { return &m_indexView; }
		inline ref<Mesh> GetMesh() { return m_mesh; }
		D3D12_SHADER_RESOURCE_VIEW_DESC GetVertexSRVDesc();
		D3D12_SHADER_RESOURCE_VIEW_DESC GetIndexSRVDesc();
		inline ComPtr<ID3D12Resource2> GetIndexResource() { return m_indexBuffer; }
		inline ComPtr<ID3D12Resource2> GetVertexResource() { return m_vertexBuffer; }
		bool Initialize(const ComPtr<ID3D12Device10>& device);
		void CopyToGPU(const ComPtr<ID3D12Resource2>& uploadBuffer, ComPtr<ID3D12GraphicsCommandList7>& uploadCommandList, UInt32 offset, Byte* mapData);
		ComPtr<ID3D12Resource2> CreateBLASResource(const ComPtr<ID3D12GraphicsCommandList7>& commandList, ComPtr<ID3D12Resource2>& scratchBuffer);
		void Release();
	private:
		D3D12_RAYTRACING_GEOMETRY_DESC GetRTGeometryDesc() const;
		ref<Mesh> m_mesh;
		ID3D12Resource2* m_vertexBuffer;
		ID3D12Resource2* m_indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_bufferView;
		D3D12_INDEX_BUFFER_VIEW m_indexView;
		ID3D12DescriptorHeap* m_indexHeap;
		ID3D12DescriptorHeap* m_vertexHeap;

		static D3D12_INPUT_ELEMENT_DESC s_inputElementDescs[3];
		static D3D12_INPUT_LAYOUT_DESC s_layoutDesc;
	};
}
