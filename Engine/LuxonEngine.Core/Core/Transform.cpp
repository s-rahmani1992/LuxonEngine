#include "Transform.h"
#include "Matrix4.h"
#include <cmath>

LuxonEngine::Transform::Transform(const Vector3& position, const Vector3& scale, const Vector3& axis, Float angleDeg)
	:m_position(position), m_scale(scale), m_axis(axis), m_angle(angleDeg)
{
	UpdateDirections();
	UpdateMatrix();
}

LuxonEngine::Transform::Transform()
	:Transform(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0)
{
}

void LuxonEngine::Transform::MoveForward(Float delta)
{
	SetPosition(m_position + (delta * m_forward));
}

void LuxonEngine::Transform::MoveRight(Float delta)
{
	SetPosition(m_position + (delta * m_right));
}

void LuxonEngine::Transform::RotateAround(const Vector3& axis, Float angleDeg)
{
	Matrix4 scaleTranslate{
		m_scale.x, 0.0f, 0.0f, m_position.x,
		0.0f, m_scale.y, 0.0f, m_position.y,
		0.0f, 0.0f, m_scale.z, m_position.z,
		0.0f, 0.0f, 0.0f, 1.0f,
	};

	Matrix4 mat = Matrix4::Rotate(axis, angleDeg) * Matrix4::Rotate(m_axis, m_angle);
	m_matrix = scaleTranslate * mat;
	m_forward = mat * Vector3(0.0f, 0.0f, 1.0f);
	m_up = mat * Vector3(0.0f, 1.0f, 0.0f);
	m_right = mat * Vector3(1.0f, 0.0f, 0.0f);

	Float cT = (mat(0, 0) + mat(1, 1) + mat(2, 2) - 1) / 2;
	Float sT = sqrtf(abs(1 - cT * cT));
	Float x = (mat(1, 2) - mat(2, 1)) / 2 * sT;
	Float y = (mat(2, 0) - mat(0, 2)) / 2 * sT;
	Float z = (mat(0, 1) - mat(1, 0)) / 2 * sT;
	m_axis = Vector3(x, y, z);
	m_angle = atan2f(sT, cT) * ( 180 / PI);
}

void LuxonEngine::Transform::Translate(const Vector3& delta)
{
	SetPosition(m_position + delta);
}

void LuxonEngine::Transform::SetOrientation(const Vector3& forward, const Vector3& up)
{
	m_forward = forward.Normalize();
	m_up = up.Normalize();
	m_right = Vector3::Cross(m_up, m_forward).Normalize();

	// Rotation matrix columns: right, up, forward (matches UpdateDirections/RotateAround,
	// where m_forward = R * (0,0,1), m_up = R * (0,1,0), m_right = R * (1,0,0)).
	float m00 = m_right.x, m01 = m_up.x, m02 = m_forward.x;
	float m10 = m_right.y, m11 = m_up.y, m12 = m_forward.y;
	float m20 = m_right.z, m21 = m_up.z, m22 = m_forward.z;

	float trace = m00 + m11 + m22;

	// acos((trace-1)/2) is always in [0, 180°], so there is no
	// sign ambiguity to resolve here.
	float cosT = (trace - 1.0f) / 2.0f;
	cosT = std::max(-1.0f, std::min(1.0f, cosT)); // clamp for float error
	float angleRad = std::acos(cosT);
	float sinT = std::sqrt(std::max(0.0f, 1.0f - cosT * cosT));

	if (sinT < 1e-6f)
	{
		if (angleRad < 1e-3f)
		{
			m_axis = { 0.0f, 1.0f, 0.0f }; // no rotation, arbitrary axis
		}
		else
		{
			// 180° rotation: extract axis from the symmetric part instead.
			Vector3 axis(
				std::sqrt(std::max(0.0f, (m00 + 1.0f) / 2.0f)),
				std::sqrt(std::max(0.0f, (m11 + 1.0f) / 2.0f)),
				std::sqrt(std::max(0.0f, (m22 + 1.0f) / 2.0f)));

			if (m01 < 0.0f) axis.y = -axis.y;
			if (m02 < 0.0f) axis.z = -axis.z;

			m_axis = axis.SquareMagnitude() > 1e-6f ? axis.Normalize() : Vector3(0.0f, 1.0f, 0.0f);
		}
	}
	else
	{
		// Matrix4::Rotate's Rodrigues formula uses the opposite skew-symmetric
		// sign convention from the standard right-hand-rule form, so the axis
		// extracted from (R - R^T) must be negated to stay self-consistent
		// with Matrix4::Rotate / UpdateDirections.
		m_axis = -Vector3(
			(m21 - m12) / (2.0f * sinT),
			(m02 - m20) / (2.0f * sinT),
			(m10 - m01) / (2.0f * sinT));
	}

	m_angle = angleRad * (180.0f / PI);

	UpdateMatrix();
}

void LuxonEngine::Transform::UpdateDirections()
{
	Matrix4 mat = Matrix4::Rotate(m_axis, m_angle);
	m_forward = mat * Vector3(0.0f, 0.0f, 1.0f);
	m_up = mat * Vector3(0.0f, 1.0f, 0.0f);
	m_right = mat * Vector3(1.0f, 0.0f, 0.0f);
}

void LuxonEngine::Transform::UpdateMatrix()
{
	m_matrix = Matrix4::Translate(m_position) * Matrix4::Rotate(m_axis, m_angle) * Matrix4::Scale(m_scale);
}
