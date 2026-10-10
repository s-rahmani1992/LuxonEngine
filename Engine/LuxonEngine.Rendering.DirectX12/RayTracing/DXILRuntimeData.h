#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <string>
#include <vector>

namespace LuxonEngine::Rendering::DX12::RayTracing {
	// values of the shader kind in the runtime data of a DXIL library
	enum class DXILShaderKind : UInt32 {
		RayGeneration = 7,
		Intersection = 8,
		AnyHit = 9,
		ClosestHit = 10,
		Miss = 11,
	};

	struct DXILFunctionData {
		std::string name; // the unmangled name of the function, the same as the entry point
		std::string mangledName;
		UInt32 shaderKind = 0;
		UInt32 payloadSize = 0; // in bytes. only for ray tracing shaders
		UInt32 attributeSize = 0; // in bytes. only for ray tracing shaders
	};

	/// <summary>
	/// Reads the functions of the runtime data (RDAT) part of a compiled DXIL library. The runtime data has the payload and attribute sizes
	/// of the ray tracing shaders, that the library reflection of D3D12 does not have. There is no public header for the layout of the part,
	/// so the reader checks the sizes and the offsets and fails if the data does not look as expected.
	/// </summary>
	/// <returns>false if the library has no runtime data or it cannot be read</returns>
	bool ReadDXILFunctions(IDxcBlob* library, std::vector<DXILFunctionData>& functions);
}
