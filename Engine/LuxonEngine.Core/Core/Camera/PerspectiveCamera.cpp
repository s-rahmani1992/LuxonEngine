#include "PerspectiveCamera.h"

QuantumEngine::PerspectiveCamera::PerspectiveCamera(const ref<Transform>& transform, Float nearZ, Float farZ, Float aspect, Float fovAngleDeg)
	:Camera(transform), m_nearZ(nearZ), m_farZ(farZ), m_aspect(aspect), m_fovAngleDeg(fovAngleDeg)
{
	m_projectionMatrix = Matrix4::PerspectiveProjection(nearZ, farZ, aspect, fovAngleDeg);
	m_inverseProjectionMatrix = Matrix4::InversePerspectiveProjection(nearZ, farZ, aspect, fovAngleDeg);
}
