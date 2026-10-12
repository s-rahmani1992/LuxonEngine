#pragma once
#include "../Core/TypeId.h"

namespace LuxonEngine::Rendering {
	using RendererTypeId = TypeId;
}

#define LUXON_RENDERER_TYPE(Type) \
	LUXON_TYPE_STATIC(Type) \
	::LuxonEngine::Rendering::RendererTypeId GetTypeId() const override \
	{ \
		LUXON_ASSERT_SELF_TYPE(Type); \
		return StaticTypeId(); \
	}
