#include "PerspectiveCamera.h"

LuxonEngine::PerspectiveCamera::PerspectiveCamera(const ref<Transform>& transform, Float nearZ, Float farZ, Float aspect, Float fovAngleDeg)
	:Camera(transform), m_nearZ(nearZ), m_farZ(farZ), m_aspect(aspect), m_fovAngleDeg(fovAngleDeg)
{
	UpdateProjectionMatrix();
}

void LuxonEngine::PerspectiveCamera::ChangeAspect(float aspect)
{
	m_aspect = aspect;
	UpdateProjectionMatrix();
}

void LuxonEngine::PerspectiveCamera::UpdateProjectionMatrix()
{
	m_projectionMatrix = Matrix4::PerspectiveProjection(m_nearZ, m_farZ, m_aspect, m_fovAngleDeg);
	m_inverseProjectionMatrix = Matrix4::InversePerspectiveProjection(m_nearZ, m_farZ, m_aspect, m_fovAngleDeg);
}
