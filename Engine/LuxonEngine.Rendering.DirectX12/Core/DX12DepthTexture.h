#pragma once
#include "DX12Texture.h"

namespace LuxonEngine::Rendering::DX12 {
	class DX12DepthTexture : public DX12Texture
	{
	public:
		DX12DepthTexture(ComPtr<ID3D12Resource2> resource, UInt32 width, UInt32 height, UInt16 mipLevels, DXGI_FORMAT format,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuDsvHandle)
			: DX12Texture(resource, width, height, mipLevels, format, cpuSrvHandle, {}), m_cpuDsvHandle(cpuDsvHandle) {}

		virtual void Release() override
		{
			DX12Texture::Release();
			m_cpuDsvHandle = {};
		}

		inline D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const { return m_cpuDsvHandle; }
		inline bool HasDsvHandle() const { return m_cpuDsvHandle.ptr != 0; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDsvHandle = {};
	};
}
