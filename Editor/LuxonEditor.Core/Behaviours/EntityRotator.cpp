#include "EntityRotator.h"

EntityRotator::EntityRotator()
	:m_transform(nullptr), m_speed(10), m_axis(0, 1, 0)
{
}

EntityRotator::EntityRotator(ref<LuxonEngine::Transform>& transform, float speed, Vector3 axis)
	:m_transform(transform), m_speed(speed), m_currentAngle(0), m_axis(axis)
{
}

void EntityRotator::Start()
{
	m_currentAngle = m_transform ? m_transform->GetAngle() : 0;
}

void EntityRotator::Update(Float deltaTime)
{
	m_transform->RotateAround(m_axis, m_speed * deltaTime);
}
