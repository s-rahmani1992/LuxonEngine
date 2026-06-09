#pragma once

#include "../BasicTypes.h"
#include "../export.h"

namespace LuxonEngine {
	struct LUXON_CORE_API Vector2UInt {
	public:
		Vector2UInt();
		Vector2UInt(UInt32 x);
		Vector2UInt(UInt32 x, UInt32 y);

	public:
		UInt32 x;
		UInt32 y;
	};
}