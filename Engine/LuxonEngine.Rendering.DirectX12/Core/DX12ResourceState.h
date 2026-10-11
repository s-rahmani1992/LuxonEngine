#pragma once
#include "pch.h"

namespace LuxonEngine::Rendering::DX12 {
	inline bool IsReadOnlyState(D3D12_RESOURCE_STATES state)
	{
		constexpr D3D12_RESOURCE_STATES readMask = D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		return state != D3D12_RESOURCE_STATE_COMMON && (state & ~readMask) == 0;
	}
}
