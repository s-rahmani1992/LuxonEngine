#include "EntityRotator.h"

EntityRotator::EntityRotator(ref<QuantumEngine::Transform>& transform, float speed)
	:m_transform(transform), m_speed(speed), m_currentAngle(0)
{
}

void EntityRotator::Update(Float deltaTime)
{
	m_transform->RotateAround(Vector3(0, 1, 0), m_speed * deltaTime);
}
