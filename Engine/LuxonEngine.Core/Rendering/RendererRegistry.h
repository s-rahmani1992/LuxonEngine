#pragma once
#include "../BasicTypes.h"
#include "../export.h"
#include "../Core/TypeRegistry.h"
#include "RendererType.h"

namespace LuxonEngine::Rendering {
	using RendererTypeInfo = TypeInfo;

	class LUXON_CORE_API RendererRegistry {
	public:
		static bool Register(RendererTypeId id, const char* name);

		template<typename T>
		static bool Register() { return Register(T::StaticTypeId(), T::StaticTypeName()); }

		static const RendererTypeInfo* Find(RendererTypeId id);
	};
}
