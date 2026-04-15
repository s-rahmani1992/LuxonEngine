#pragma once
#include <vector>
#include <Core/Light/Lights.h>

namespace QuantumEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	class DX12LightManager
	{
	public:
		bool Initialize(const SceneLightData& lights, const ComPtr<ID3D12Device10>& device);
		inline ComPtr<ID3D12DescriptorHeap> GetDescriptor() { return m_lightHeap; }
		inline ComPtr<ID3D12Resource2> GetResource() { return m_lightBuffer; }
	private:
		ComPtr<ID3D12Resource2> m_lightBuffer;
		ComPtr<ID3D12DescriptorHeap> m_lightHeap;
	};
}