#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "DX12GraphicContext.h"
#include <array>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Renderer;
	class ShaderProgram;
}

namespace LuxonEngine::Rendering::DX12 {
	class DX12PipelineFactory;
	class DX12GPUResourceManager;
	class DX12AssetManager;
	class DX12RenderPass;
	class DX12ComputePass;

	struct DX12RendererEntity {
		GameEntity* entity = nullptr;
		Renderer* renderer = nullptr;
		DX12EntityGPUData* gpuData = nullptr;
	};

	struct DX12NamedDescriptor {
		const char* name = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
	};

	struct DX12PassSetupContext {
		DX12PipelineFactory* pipelineFactory = nullptr;
		DX12GPUResourceManager* resourceManager = nullptr;

		DX12AssetManager* assetManager = nullptr;
		std::function<ref<ShaderProgram>(const std::string&)> getInternalProgram; // the programs registered by the engine, null when not found

		UInt32 renderWidth = 0;
		UInt32 renderHeight = 0;
		DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
		std::array<float, 4> backgroundColor = {};

		std::span<DX12EntityGPUData> allEntities;

		D3D12_CPU_DESCRIPTOR_HANDLE cameraDescriptor = {};
		D3D12_CPU_DESCRIPTOR_HANDLE lightDescriptor = {};

		std::vector<DX12NamedDescriptor> globalDescriptors;

		DX12ComputePass* computePass = nullptr;
		DX12RenderPass* mainRenderPass = nullptr;

		DX12RenderPass* gBufferRenderPass = nullptr;
	};

	class IDX12RendererSetup {
	public:
		virtual ~IDX12RendererSetup() = default;

		virtual void Setup(std::span<DX12RendererEntity> entities, DX12PassSetupContext& context) = 0;

		virtual void Update() {}

		virtual void Teardown() {}
	};
}
