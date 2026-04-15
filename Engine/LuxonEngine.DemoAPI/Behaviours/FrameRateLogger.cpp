#include "FrameRateLogger.h"
#include "Platform/Application.h"

void FrameRateLogger::Update(Float deltaTime)
{
	if (deltaTime < 0.0001f)
		return;
	m_totalFPS += 1.0f / deltaTime;
	m_frameCount++;
	OutputDebugStringA((std::to_string(m_totalFPS/ m_frameCount) + '\n').c_str());
}
