#pragma once
#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>

using namespace QuantumEngine;

class EntityPositionController : public QuantumEngine::Behaviour
{
public:
	EntityPositionController(ref<QuantumEngine::Transform>& transform, Float speed);
	virtual void Update(Float deltaTime) override;
private:
	ref<QuantumEngine::Transform> m_transform;
	Vector3 m_currentPosition;
	Float m_speed;
};