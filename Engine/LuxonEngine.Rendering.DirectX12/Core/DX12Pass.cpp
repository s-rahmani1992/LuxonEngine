#include "pch.h"
#include "DX12Pass.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12ResourceState.h"
#include <Core/Logger.h>

namespace LuxonEngine::Rendering::DX12 {
	void DX12Pass::Use(DX12Texture* texture, D3D12_RESOURCE_STATES state)
	{
		if (texture == nullptr || texture->GetResource() == nullptr)
			return;

		AddUsage(DX12ResourceUsage{ .resource = texture->GetResource(), .texture = texture, .state = state });
	}

	void DX12Pass::Use(DX12Buffer* buffer, D3D12_RESOURCE_STATES state)
	{
		if (buffer == nullptr || buffer->GetResource() == nullptr)
			return;

		AddUsage(DX12ResourceUsage{ .resource = buffer->GetResource(), .buffer = buffer, .state = state });
	}

	void DX12Pass::UseBackBuffer(D3D12_RESOURCE_STATES state)
	{
		if (m_usesBackBuffer && m_backBufferState != state) {
			Logger::LogError("A pass declares two different states for the back buffer, the first one is used");
			return;
		}

		m_usesBackBuffer = true;
		m_backBufferState = state;
	}

	void DX12Pass::AddUsage(const DX12ResourceUsage& usage)
	{
		for (auto& existing : m_usages) {
			if (existing.resource != usage.resource)
				continue;

			if (existing.state == usage.state)
				return;

			if (IsReadOnlyState(existing.state) && IsReadOnlyState(usage.state))
				existing.state |= usage.state;
			else
				Logger::LogError("A pass declares two states that cannot be combined for the same resource, the first one is used");

			return;
		}

		m_usages.push_back(usage);
	}

	void DX12Pass::ClearUsages()
	{
		m_usages.clear();
		m_usesBackBuffer = false;
		m_backBufferState = D3D12_RESOURCE_STATE_COMMON;
	}
}
