#include "EntityMover.h"

EntityMover::EntityMover(ref<QuantumEngine::Transform>& transform, Vector3 point1, Vector3 point2, float start, float speed)
	:m_transform(transform), m_currentTargetIndex(0), m_currentPoint(start)
{
	m_points[0] = point1;
	m_points[1] = point2;
	m_currentTarget = m_points[m_currentTargetIndex];
	m_currentPosition = m_currentPoint * point1 + (1 - m_currentPoint) * point2;
	m_speed = speed * ((m_currentTarget - m_currentPosition).Normalize());
}

void EntityMover::Update(Float deltaTime)
{
	m_currentPosition += deltaTime * m_speed;

	if ((m_currentPosition - m_currentTarget).SquareMagnitude() < 0.01f) {
		m_currentPosition = m_currentTarget;
		m_speed = -m_speed;
		m_currentTargetIndex = (m_currentTargetIndex + 1) % 2;
		m_currentTarget = m_points[m_currentTargetIndex];
	}

	m_transform->SetPosition(m_currentPosition);
}
