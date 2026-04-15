#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Behaviour {
	public:
		virtual ~Behaviour() = default;
		virtual void Update(Float deltaTime) = 0;
	};
}