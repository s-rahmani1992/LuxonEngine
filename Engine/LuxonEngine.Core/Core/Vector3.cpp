#include "Vector3.h"

QuantumEngine::Vector3::Vector3()
	:Vector3(0.0f)
{
}

QuantumEngine::Vector3::Vector3(Float x)
	:Vector3(x, x, x)
{
}

QuantumEngine::Vector3::Vector3(Float x, Float y, Float z)
	:x(x), y(y), z(z)
{
}

QuantumEngine::Vector3 QuantumEngine::Vector3::Normalize() const
{
	Float m = Magnitude();

	if (m == 0.0f)
		return Vector3(0.0f);

	return Vector3(x / m, y / m, z / m);
}

Float QuantumEngine::Vector3::Magnitude() const
{
	return sqrtf(SquareMagnitude());
}

Float QuantumEngine::Vector3::SquareMagnitude() const
{
	return (x * x) + (y * y) + (z * z);
}

Float QuantumEngine::Vector3::Dot(const Vector3& vectorA, const Vector3& vectorB)
{
	return (vectorA.x * vectorB.x) + (vectorA.y * vectorB.y) + (vectorA.z * vectorB.z);
}

QuantumEngine::Vector3 QuantumEngine::Vector3::operator-()
{
	return Vector3(-x, -y, -z);
}

QuantumEngine::Vector3 QuantumEngine::Vector3::operator+(const Vector3& vectorB)
{
	return Vector3(x + vectorB.x, y + vectorB.y, z + vectorB.z);
}

QuantumEngine::Vector3 QuantumEngine::Vector3::operator+=(const Vector3& vectorB)
{
	x += vectorB.x;
	y += vectorB.y;
	z += vectorB.z;
	return *this;
}

QuantumEngine::Vector3 QuantumEngine::Vector3::operator-(const Vector3& vectorB)
{
	return Vector3(x - vectorB.x, y - vectorB.y, z - vectorB.z);
}

QuantumEngine::Vector3 QuantumEngine::Vector3::operator-=(const Vector3& vectorB)
{
	x -= vectorB.x;
	y -= vectorB.y;
	z -= vectorB.z;
	return *this;
}

QuantumEngine::Vector3 QuantumEngine::Vector3::operator*(Float fValue)
{
	return Vector3(fValue * x, fValue * y, fValue * z);
}

std::string QuantumEngine::Vector3::ToString() const
{
	return "( " + std::to_string(x) + " , " + std::to_string(y) + " , " + std::to_string(z) + ")";
}

QuantumEngine::Vector3 operator*(Float fValue, const QuantumEngine::Vector3& vector)
{
	return QuantumEngine::Vector3(fValue * vector.x, fValue * vector.y, fValue * vector.z);
}
