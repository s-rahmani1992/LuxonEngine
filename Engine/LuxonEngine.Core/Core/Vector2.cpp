#include "Vector2.h"

QuantumEngine::Vector2::Vector2() 
	: Vector2(0.0f)
{
}

QuantumEngine::Vector2::Vector2(Float x)
	:Vector2(x, x)
{
}

QuantumEngine::Vector2::Vector2(Float x, Float y)
	:x(x), y(y)
{
}