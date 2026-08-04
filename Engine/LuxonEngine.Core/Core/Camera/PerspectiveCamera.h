#pragma once
#include "Camera.h"

namespace LuxonEngine {
	class LUXON_CORE_API PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(const ref<Transform>& transform, Float nearZ, Float farZ, Float aspect, Float fovAngleDeg);
		void ChangeAspect(float aspect);
		inline Float GetFovAngle() const { return m_fovAngleDeg; }
		inline Float GetNearZ() const { return m_nearZ; }
		inline Float GetFarZ() const { return m_farZ; }
		inline void SetFovAngle(float fovAngleDeg) { m_fovAngleDeg = fovAngleDeg; UpdateProjectionMatrix(); }
		inline void SetNearZ(Float nearZ) { m_nearZ = nearZ; UpdateProjectionMatrix(); }
		inline void SetFarZ(Float farZ) { m_farZ = farZ; UpdateProjectionMatrix(); }
	private:
		void UpdateProjectionMatrix();

		Float m_nearZ;
		Float m_farZ;
		Float m_aspect;
		Float m_fovAngleDeg;
	};
}