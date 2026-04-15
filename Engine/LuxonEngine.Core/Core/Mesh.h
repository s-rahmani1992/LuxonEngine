#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include <vector>

namespace QuantumEngine::Rendering {
	class GPUMeshController;
}

namespace QuantumEngine {
	struct Vertex {
		Vector3 position;
		Vector2 uv;
		Vector3 normal;

		Vertex() : Vertex(Vector3(0.0f), Vector2(0.0f), Vector3(0.0f))
		{
		}

		Vertex(const Vector3& pos, const Vector2& tex, const Vector3& normal)
			:position(pos), uv(tex), normal(normal)
		{
		}
	};

	class Mesh {
	public:
		Mesh(const std::vector<Vertex>& vertices, const std::vector<UInt32>& indices);
		UInt32 GetVertexCount() const { return m_vertices.size(); }
		UInt32 GetIndexCount() const { return m_indices.size(); }
		UInt32 GetTotalSize() const { return m_vertices.size() * sizeof(Vertex) + m_indices.size() * sizeof(UInt32); }
		void CopyVertexData(Byte* dest);
		void CopyIndexData(Byte* dest);
		ref<Rendering::GPUMeshController> GetGPUHandle() { return m_gpuHandle; }
		void SetGPUHandle(ref<Rendering::GPUMeshController> gpuHandle) { m_gpuHandle = gpuHandle; }
		bool IsUploadedToGPU() const { return m_gpuHandle != nullptr; }
		void Release();
	private:
		std::vector<Vertex> m_vertices;
		std::vector<UInt32> m_indices;
		ref<Rendering::GPUMeshController> m_gpuHandle;
	};
}