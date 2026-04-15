#pragma once
#include "DX12GraphicContext.h"

namespace QuantumEngine::Rendering::DX12
{
	namespace RayTracing {
		class DX12RayTracingPipelineModule;
	}

	class DX12RayTracingContext : public DX12GraphicContext
	{
	public:
		DX12RayTracingContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window) 
			: DX12GraphicContext(bufferCount, m_commandExecuter, window) {}
		
		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		void PrepareRayTracingPipeline(const ref<Material>& rtGlobalMaterial);

	private:
		std::vector<DX12RayTracingGPUData> m_rtEntityData;
		ref<RayTracing::DX12RayTracingPipelineModule> m_rayTracingPipeline;
	};
}


