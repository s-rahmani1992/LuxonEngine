#include "Mesh.h"

LuxonEngine::Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<UInt32>& indices)
	: m_vertices(vertices), m_indices(indices)
{

}

void LuxonEngine::Mesh::CopyIndexData(Byte* dest)
{
	std::memcpy(dest, m_indices.data(), m_indices.size() * sizeof(UInt32));
}

void LuxonEngine::Mesh::CopyVertexData(Byte* dest)
{
	std::memcpy(dest, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
}

void LuxonEngine::Mesh::Release()
{
	m_vertices.clear();
	m_indices.clear();
}
