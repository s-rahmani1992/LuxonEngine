#pragma once
#include "../BasicTypes.h"
#include "../export.h"

namespace LuxonEngine {
	struct LUXON_CORE_API Vector2 {
	public:
		Vector2();
		Vector2(Float x);
		Vector2(Float x, Float y);

	public:
		Float x;
		Float y;
	};
}