#include "ShapeBuilder.h"
#include "Mesh.h"

ref<LuxonEngine::Mesh> LuxonEngine::ShapeBuilder::CreateCube(Float size)
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

ref<LuxonEngine::Mesh> LuxonEngine::ShapeBuilder::CreateCompleteCube(Float size)
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

ref<LuxonEngine::Mesh> LuxonEngine::ShapeBuilder::CreateSphere(Float radius, UInt32 hSegments, UInt32 vSegment)
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

ref<LuxonEngine::Mesh> LuxonEngine::ShapeBuilder::CreatePlane(float uFactor, float vFactor, UInt32 uSegments, UInt32 vSegments)
{
    std::vector<Vertex> planeVertices;
    for (UInt32 v = 0; v <= vSegments; ++v) {
        for (UInt32 u = 0; u <= uSegments; ++u) {
            float x = (float)u / uSegments;
            float y = (float)v / vSegments;

            float calculatedU = x * uFactor;
            float calculatedV = y * vFactor;
            planeVertices.push_back(Vertex(Vector3(2*x - 1, 0.0f, 2*y - 1), Vector2(calculatedU, calculatedV), Vector3(0.0f, 1.0f, 0.0f)));
        }
    }
    std::vector<UInt32> planeIndices;
    for (UInt32 v = 0; v < vSegments; ++v) {
        for (UInt32 u = 0; u < uSegments; ++u) {
            UInt32 startIndex = v * (uSegments + 1) + u;
            planeIndices.push_back(startIndex);
            planeIndices.push_back(startIndex + 1);
            planeIndices.push_back(startIndex + uSegments + 1);
            planeIndices.push_back(startIndex + 1);
            planeIndices.push_back(startIndex + uSegments + 2);
            planeIndices.push_back(startIndex + uSegments + 1);
        }
    }
	return std::make_shared<Mesh>(planeVertices, planeIndices);
}
