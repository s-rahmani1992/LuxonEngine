#pragma once

#include "../BasicTypes.h"

namespace QuantumEngine {
	struct Vector2UInt {
	public:
		Vector2UInt();
		Vector2UInt(UInt32 x);
		Vector2UInt(UInt32 x, UInt32 y);

	public:
		UInt32 x;
		UInt32 y;
	};
}