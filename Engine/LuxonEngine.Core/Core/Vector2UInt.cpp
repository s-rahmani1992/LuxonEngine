#include "Vector2UInt.h"

QuantumEngine::Vector2UInt::Vector2UInt()
	: Vector2UInt(0)
{
}

QuantumEngine::Vector2UInt::Vector2UInt(UInt32 x)
	:Vector2UInt(x, x)
{
}

QuantumEngine::Vector2UInt::Vector2UInt(UInt32 x, UInt32 y)
	:x(x), y(y)
{
}
