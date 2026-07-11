#pragma once
#include "Camera.h"

namespace LuxonEngine {
	class LUXON_CORE_API PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(const ref<Transform>& transform, Float nearZ, Float farZ, Float aspect, Float fovAngleDeg);
		void ChangeAspect(float aspect);
	private:
		Float m_nearZ;
		Float m_farZ;
		Float m_aspect;
		Float m_fovAngleDeg;
	};
}