#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine
{
	class Mesh;
	class ShapeBuilder
	{
	public:
		static ref<Mesh> CreateCube(Float size);
		static ref<Mesh> CreateCompleteCube(Float size);
		static ref<Mesh> CreateSphere(Float radius, UInt32 hSegments, UInt32 vSegment);
	};
}