#include "Transform.h"
#include "Matrix4.h"

QuantumEngine::Transform::Transform(const Vector3& position, const Vector3& scale, const Vector3& axis, Float angleDeg)
	:m_position(position), m_scale(scale), m_axis(axis), m_angle(angleDeg)
{
	UpdateDirections();
	UpdateMatrix();
}

QuantumEngine::Transform::Transform()
	:Transform(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0)
{
}

void QuantumEngine::Transform::MoveForward(Float delta)
{
	SetPosition(m_position + (delta * m_forward));
}

void QuantumEngine::Transform::MoveRight(Float delta)
{
	SetPosition(m_position + (delta * m_right));
}

void QuantumEngine::Transform::RotateAround(const Vector3& axis, Float angleDeg)
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

void QuantumEngine::Transform::Translate(const Vector3& delta)
{
	SetPosition(m_position + delta);
}

void QuantumEngine::Transform::UpdateDirections()
{
	Matrix4 mat = Matrix4::Rotate(m_axis, m_angle);
	m_forward = mat * Vector3(0.0f, 0.0f, 1.0f);
	m_up = mat * Vector3(0.0f, 1.0f, 0.0f);
	m_right = mat * Vector3(1.0f, 0.0f, 0.0f);
}

void QuantumEngine::Transform::UpdateMatrix()
{
	Matrix4 ST{
		m_scale.x, 0.0f, 0.0f, m_position.x,
		0.0f, m_scale.y, 0.0f, m_position.y,
		0.0f, 0.0f, m_scale.z, m_position.z,
		0.0f, 0.0f, 0.0f, 1.0f,
	};

	m_matrix = ST * Matrix4::Rotate(m_axis, m_angle);
}
