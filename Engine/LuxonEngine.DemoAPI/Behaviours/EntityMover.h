#pragma once

#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>

using namespace QuantumEngine;

class EntityMover : public QuantumEngine::Behaviour
{
public:
	EntityMover(ref<QuantumEngine::Transform>& transform, Vector3 point1, Vector3 point2, float start, float speed);
	virtual void Update(Float deltaTime) override;
private:
	ref<QuantumEngine::Transform> m_transform;
	Vector3 m_points[2];
	Vector3 m_currentPosition;
	UInt8 m_currentTargetIndex;
	Float m_currentPoint;
	Vector3 m_speed;
	Vector3 m_currentTarget;
};

