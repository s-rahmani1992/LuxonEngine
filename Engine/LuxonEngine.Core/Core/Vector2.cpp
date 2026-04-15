#include "Vector2.h"

LuxonEngine::Vector2::Vector2() 
	: Vector2(0.0f)
{
}

LuxonEngine::Vector2::Vector2(Float x)
	:Vector2(x, x)
{
}

LuxonEngine::Vector2::Vector2(Float x, Float y)
	:x(x), y(y)
{
}