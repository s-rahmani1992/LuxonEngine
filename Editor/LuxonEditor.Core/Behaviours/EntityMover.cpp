#include "EntityMover.h"

EntityMover::EntityMover()
	:m_currentTargetIndex(0), m_currentPoint(1.0f), m_speedMagnitude(1.0f), m_start(1.0f)
{
}

EntityMover::EntityMover(ref<LuxonEngine::Transform>& transform, Vector3 point1, Vector3 point2, float start, float speed)
	:m_transform(transform), m_currentTargetIndex(0), m_start(start), m_speedMagnitude(speed), m_currentPoint(start)
{
	m_points[0] = point1;
	m_points[1] = point2;
}

void EntityMover::Start()
{
	m_currentPosition = m_currentPoint * m_points[1] + (1 - m_currentPoint) * m_points[0];
	m_currentTarget = m_points[m_currentTargetIndex];
	m_speed = m_speedMagnitude * ((m_currentTarget - m_currentPosition).Normalize());
	m_transform->SetOrientation((m_currentTarget - m_currentPosition).Normalize(), Vector3(0, 1, 0));

}

void EntityMover::Update(Float deltaTime)
{
	m_currentPosition += deltaTime * m_speed;

	if ((m_currentPosition - m_currentTarget).SquareMagnitude() < 0.01f) {
		m_currentPosition = m_currentTarget;
		m_speed = -m_speed;
		m_currentTargetIndex = (m_currentTargetIndex + 1) % 2;
		m_currentTarget = m_points[m_currentTargetIndex];
		m_transform->SetOrientation((m_currentTarget - m_currentPosition).Normalize(), Vector3(0, 1, 0));
	}

	m_transform->SetPosition(m_currentPosition);
}
