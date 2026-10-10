#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12PipelineFactory.h"
#include <string>
#include <vector>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12::RayTracing {
	class HLSLRayTracingProgram;

	class RTStateObjectBuilder {
	public:
		static ComPtr<ID3D12StateObject> Create(ID3D12Device10* device, const RayTracingPipelineProperties& properties,
			HLSLRayTracingProgram* globalProgram, const std::vector<HLSLRayTracingProgram*>& localPrograms, std::string& error);
	};
}
