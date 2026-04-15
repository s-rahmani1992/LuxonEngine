#include "ShapeBuilder.h"
#include "Mesh.h"

ref<QuantumEngine::Mesh> QuantumEngine::ShapeBuilder::CreateCube(Float size)
{
    std::vector<Vertex> cubeVertices = {
        Vertex(size * Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(-1.0f, -1.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, -1.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, -1.0f), Vector2(1.0f, 0.0f), Vector3(1.0f, 1.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.0f, 0.0f), Vector3(-1.0f, 1.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, -1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(-1.0f, -1.0f, 1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(1.0f, -1.0f, 1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(-1.0f, 1.0f, 1.0f).Normalize()),
    };

    std::vector<UInt32> cubeIndices = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        2, 5, 6, 2, 1, 5,
        0, 7, 4, 0, 3, 7,
        3, 6, 7, 3, 2, 6,
        1, 4, 5, 1, 0, 4,
    };

    return std::make_shared<Mesh>(cubeVertices, cubeIndices);
}

ref<QuantumEngine::Mesh> QuantumEngine::ShapeBuilder::CreateCompleteCube(Float size)
{
    std::vector<Vertex> cubeVertices = {
        Vertex(size * Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f).Normalize()),
        
        Vertex(size * Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f).Normalize()),
       
        Vertex(size * Vector3(1.0f, -1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f).Normalize()),
        
        Vertex(size * Vector3(-1.0f, -1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f).Normalize()),
        
        Vertex(size * Vector3(-1.0f, -1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, -1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f).Normalize()),

        Vertex(size * Vector3(-1.0f, 1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(1.0f, 1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f).Normalize()),
        Vertex(size * Vector3(-1.0f, 1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f).Normalize()),
    };

    std::vector<UInt32> cubeIndices = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        8, 9, 10, 8, 10, 11,
        12, 14, 13, 12, 15, 14,
        16, 17, 18, 16, 18, 19,
        20, 22, 21, 20, 23, 22,
    };

    return std::make_shared<Mesh>(cubeVertices, cubeIndices);
}

ref<QuantumEngine::Mesh> QuantumEngine::ShapeBuilder::CreateSphere(Float radius, UInt32 hSegments, UInt32 vSegment)
{
	std::vector<Vertex> sphereVertices;
	for (int stacks = 0; stacks < hSegments; stacks++) {
		float phi = (stacks / (float)(hSegments - 1)) * (float)PI;
		for (int slices = 0; slices <= hSegments; slices++) {
			float theta = (slices / (float)hSegments) * 2 * (float)PI;
			Vector3 normal(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
			sphereVertices.push_back(Vertex(
				radius * normal,
				Vector2(slices / (float)vSegment, stacks / (float)hSegments),
				normal
			));
		}
	}

	std::vector<UInt32> sphereIndices;
	for (int stacks = 0; stacks < hSegments; stacks++) {
		for (int slices = 0; slices < vSegment; slices++) {
			unsigned int nextSlice = slices + 1;
			unsigned int nextStack = (stacks + 1) % hSegments;

			unsigned int index0 = stacks * (vSegment + 1) + slices;
			unsigned int index1 = nextStack * (vSegment + 1) + slices;
			unsigned int index2 = stacks * (vSegment + 1) + nextSlice;
			unsigned int index3 = nextStack * (vSegment + 1) + nextSlice;
			sphereIndices.insert(sphereIndices.end(), { index0, index2, index1, index2, index3, index1 });
		}
	}
	return std::make_shared<Mesh>(sphereVertices, sphereIndices);
}
