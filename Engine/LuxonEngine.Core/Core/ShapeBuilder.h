#pragma once
#include "../BasicTypes.h"
#include "../export.h"

namespace LuxonEngine
{
	class Mesh;
	class LUXON_CORE_API ShapeBuilder
	{
	public:
		static ref<Mesh> CreateCube(Float size);
		static ref<Mesh> CreateCompleteCube(Float size);
		static ref<Mesh> CreateSphere(Float radius, UInt32 hSegments, UInt32 vSegment);
		static ref<Mesh> CreatePlane(float uFactor, float vFactor, UInt32 uSegments, UInt32 vSegments);
	};
}