#pragma once
#include "../BasicTypes.h"

namespace LuxonEngine {
	class Behaviour {
	public:
		virtual ~Behaviour() = default;
		virtual void Update(Float deltaTime) = 0;
	};
}