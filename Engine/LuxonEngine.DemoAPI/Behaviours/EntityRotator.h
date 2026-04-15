#pragma once
#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>

using namespace LuxonEngine;

class EntityRotator : public LuxonEngine::Behaviour
{
public:
	EntityRotator(ref<LuxonEngine::Transform>& transform, float speed);
	virtual void Update(Float deltaTime) override;
private:
	ref<LuxonEngine::Transform> m_transform;
	Float m_currentAngle;
	Float m_speed;
};
