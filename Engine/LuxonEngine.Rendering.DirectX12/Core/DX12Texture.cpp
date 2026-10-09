#include "pch.h"
#include "DX12Texture.h"

namespace LuxonEngine::Rendering::DX12 {
	DX12Texture::~DX12Texture()
	{
		Release();
	}

	void DX12Texture::Release()
	{
		m_resource.Reset();
		m_cpuSrvHandle = {};
		m_cpuUavHandle = {};
		m_width = 0;
		m_height = 0;
		m_mipLevels = 0;
		m_format = DXGI_FORMAT_UNKNOWN;
		m_state = D3D12_RESOURCE_STATE_COMMON;
	}
}
