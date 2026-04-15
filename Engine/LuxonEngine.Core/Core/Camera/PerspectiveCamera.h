#pragma once
#include "Camera.h"

namespace QuantumEngine {
	class PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(const ref<Transform>& transform, Float nearZ, Float farZ, Float aspect, Float fovAngleDeg);
	
	private:
		Float m_nearZ;
		Float m_farZ;
		Float m_aspect;
		Float m_fovAngleDeg;
	};
}