#pragma once
#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>

using namespace LuxonEngine;

class EntityPositionController : public LuxonEngine::Behaviour
{
public:
	EntityPositionController(ref<LuxonEngine::Transform>& transform, Float speed);
	virtual void Update(Float deltaTime) override;
private:
	ref<LuxonEngine::Transform> m_transform;
	Vector3 m_currentPosition;
	Float m_speed;
};