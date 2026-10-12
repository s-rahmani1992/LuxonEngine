#pragma once
#include "DX12Texture.h"

namespace LuxonEngine::Rendering::DX12 {
	class DX12RenderTexture : public DX12Texture
	{
	public:
		DX12RenderTexture(ComPtr<ID3D12Resource2> resource, UInt32 width, UInt32 height, DXGI_FORMAT format,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuUavHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuRtvHandle)
			: DX12Texture(resource, width, height, format, cpuSrvHandle, cpuUavHandle), m_cpuRtvHandle(cpuRtvHandle) {}

		virtual void Release() override
		{
			DX12Texture::Release();
			m_cpuRtvHandle = {};
		}

		inline D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return m_cpuRtvHandle; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuRtvHandle = {};
	};
}
