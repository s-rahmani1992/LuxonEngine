#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Color {
	public:
		Color(Float r, Float g, Float b, Float a);
		Color();
		inline Float* GetColorArray() { return m_colorValues; }
	private:
		Float m_colorValues[4];
	};

}