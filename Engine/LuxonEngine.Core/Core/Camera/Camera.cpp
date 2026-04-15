#include "Camera.h"
#include "../Transform.h"

QuantumEngine::Camera::Camera(const ref<Transform>& transform)
	:m_transform(transform)
{
}

QuantumEngine::Matrix4 QuantumEngine::Camera::ViewMatrix()
{
	return Matrix4::Rotate(m_transform->RotationAxis(), -m_transform->GetAngle()) * Matrix4::Translate(-m_transform->Position());
}
