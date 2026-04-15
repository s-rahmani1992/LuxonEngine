#pragma once
#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>

using namespace QuantumEngine;

class EntityRotator : public QuantumEngine::Behaviour
{
public:
	EntityRotator(ref<QuantumEngine::Transform>& transform, float speed);
	virtual void Update(Float deltaTime) override;
private:
	ref<QuantumEngine::Transform> m_transform;
	Float m_currentAngle;
	Float m_speed;
};
