#include "Camera.h"
#include "../Transform.h"

LuxonEngine::Camera::Camera(const ref<Transform>& transform)
	:m_transform(transform)
{
}

LuxonEngine::Matrix4 LuxonEngine::Camera::ViewMatrix()
{
	return Matrix4::Rotate(m_transform->RotationAxis(), -m_transform->GetAngle()) * Matrix4::Translate(-m_transform->Position());
}
