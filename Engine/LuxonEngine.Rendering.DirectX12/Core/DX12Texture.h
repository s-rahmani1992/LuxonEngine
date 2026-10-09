#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <optional>
#include <string>

namespace LuxonEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;

	struct DX12TextureDesc
	{
		UInt32 width = 1;
		UInt32 height = 1;
		UInt16 mipLevels = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
		std::optional<D3D12_CLEAR_VALUE> clearValue;
		std::wstring name;
	};

	class DX12Texture
	{
	public:
		DX12Texture(ComPtr<ID3D12Resource2> resource, UInt32 width, UInt32 height, UInt16 mipLevels, DXGI_FORMAT format,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuUavHandle)
			: m_resource(resource), m_width(width), m_height(height), m_mipLevels(mipLevels), m_format(format),
			m_cpuSrvHandle(cpuSrvHandle), m_cpuUavHandle(cpuUavHandle) {}
		virtual ~DX12Texture();

		DX12Texture(const DX12Texture&) = delete;
		DX12Texture& operator=(const DX12Texture&) = delete;

		virtual void Release();

		inline ID3D12Resource2* GetResource() const { return m_resource.Get(); }
		inline UInt32 GetWidth() const { return m_width; }
		inline UInt32 GetHeight() const { return m_height; }
		inline UInt16 GetMipLevels() const { return m_mipLevels; }
		inline DXGI_FORMAT GetFormat() const { return m_format; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return m_cpuSrvHandle; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE GetUavHandle() const { return m_cpuUavHandle; }
		inline bool HasSrvHandle() const { return m_cpuSrvHandle.ptr != 0; }
		inline bool HasUavHandle() const { return m_cpuUavHandle.ptr != 0; }

		// State as last recorded by the user of this texture. The texture never issues barriers itself.
		inline D3D12_RESOURCE_STATES GetState() const { return m_state; }
		inline void SetState(D3D12_RESOURCE_STATES state) { m_state = state; }

	private:
		ComPtr<ID3D12Resource2> m_resource;
		UInt32 m_width = 0;
		UInt32 m_height = 0;
		UInt16 m_mipLevels = 0;
		DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuSrvHandle = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuUavHandle = {};
		D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_COMMON;
	};
}
