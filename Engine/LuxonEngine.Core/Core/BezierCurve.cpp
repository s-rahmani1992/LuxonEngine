#include "BezierCurve.h"

QuantumEngine::Core::BezierCurve::BezierCurve(const Vector3& point1, const Vector3& point2, const Vector3& point3)
	: m_point1(point1), m_point2(point2), m_point3(point3)
{
}

void QuantumEngine::Core::BezierCurve::Interpolate(Float t, Vector3* position, Vector3* tangent) const
{
	*position = powf(1 - t, 2) * m_point1 + 2 * t * (1 - t) * m_point2 + powf(t, 2) * m_point3;
	*tangent = -2 * (1 - t) * m_point1 + (-4 * t + 2) * m_point2 + 2 * t * m_point3;
}

Float QuantumEngine::Core::BezierCurve::InterpolateLength(Float t) const
{
	return Integral(t) - Integral(0);
}

Float QuantumEngine::Core::BezierCurve::Integral(Float t) const
{
	auto qt = 2.0f * m_point1 - 4.0f * m_point2 + 2.0f * m_point3;
	auto q = -2.0f * m_point1 + 2.0f * m_point2;
	Float A = Vector3::Dot(qt, qt);
	Float B = 2.0f * Vector3::Dot(qt, q);
	Float C = Vector3::Dot(q, q);
	Float h = B / (2 * A);
	Float k = C - ((B * B) / (4 * A));
	Float exp = sqrtf((A * t * t) + (B * t) + C);
	return (0.5f * (t + h) * exp) +
		((0.5f * k) / sqrtf(A)) * log((t + h) + exp / sqrtf(A));
}
