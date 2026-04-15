#include "CurveModifier.h"
#include "Platform/CommonWin.h"

CurveModifier::CurveModifier(const ref<Rendering::SplineRenderer>& spline, float speed)
	:m_spline(spline), m_currentPoint(spline->GetCurve().m_point1), m_speedSign(speed)
{
}

void CurveModifier::Update(Float deltaTime)
{
	if (GetKeyState('Q') & 0x80) {
		m_currentPoint += Vector3(deltaTime * m_speedSign, 0.0f, 0.0f);
		m_spline->GetCurve().m_point1 = m_currentPoint;
		m_spline->SetDirty();
	}

	if (GetKeyState('E') & 0x80) {
		m_currentPoint -= Vector3(deltaTime * m_speedSign, 0.0f, 0.0f);
		m_spline->GetCurve().m_point1 = m_currentPoint;
		m_spline->SetDirty();
	}
}
