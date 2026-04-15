#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	struct Vector2 {
	public:
		Vector2();
		Vector2(Float x);
		Vector2(Float x, Float y);

	public:
		Float x;
		Float y;
	};
}