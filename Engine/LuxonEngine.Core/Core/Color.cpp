#include "Color.h"

LuxonEngine::Color::Color(Float r, Float g, Float b, Float a)
{
	m_colorValues[0] = r;
	m_colorValues[1] = g;
	m_colorValues[2] = b;
	m_colorValues[3] = a;
}

LuxonEngine::Color::Color()
	:Color(1.0f, 1.0f, 1.0f, 1.0f)
{
}
