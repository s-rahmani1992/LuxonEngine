#pragma once
#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>
#include "Rendering/SplineRenderer.h"

using namespace QuantumEngine;

class CurveModifier : public QuantumEngine::Behaviour
{
public:
	CurveModifier(const ref<Rendering::SplineRenderer>& spline, float speed);
	virtual void Update(Float deltaTime) override;
private:
	ref<Rendering::SplineRenderer> m_spline;
	float m_speedSign = 1.0f;
	Vector3 m_currentPoint;
};

