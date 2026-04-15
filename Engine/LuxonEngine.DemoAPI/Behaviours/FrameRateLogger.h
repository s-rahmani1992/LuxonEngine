#pragma once
#include "Core/Behaviour.h"

class FrameRateLogger : public QuantumEngine::Behaviour
{
public:
	virtual void Update(Float deltaTime) override;
private:
	double m_totalFPS = 0;
	UInt32 m_frameCount = 0;
};

