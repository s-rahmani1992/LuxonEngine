#include "Vector2UInt.h"

LuxonEngine::Vector2UInt::Vector2UInt()
	: Vector2UInt(0)
{
}

LuxonEngine::Vector2UInt::Vector2UInt(UInt32 x)
	:Vector2UInt(x, x)
{
}

LuxonEngine::Vector2UInt::Vector2UInt(UInt32 x, UInt32 y)
	:x(x), y(y)
{
}
