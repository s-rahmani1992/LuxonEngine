#include "Matrix4.h"
#include "Vector3.h"

QuantumEngine::Matrix4::Matrix4(const std::initializer_list<Float>& values)
{
	std::copy(values.begin(), values.end(), m_values);
}

QuantumEngine::Matrix4::Matrix4()
	:Matrix4({
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
		})
{
}

QuantumEngine::Matrix4 QuantumEngine::Matrix4::operator*(const Matrix4& matrixB)
{
	Float newMat[16];

	for (int r = 0; r < 4; r++) {
		int rIndex = 4 * r;

		for (int c = 0; c < 4; c++) {
			float m = 0.0f;
			int cIndex = c;
			for (int i = 0; i < 4; i++) {
				m += m_values[rIndex + i] * matrixB.m_values[cIndex];
				cIndex += 4;
			}

			newMat[4 * r + c] = m;
		}
	}
	Matrix4 mat;
	std::memcpy(mat.m_values, newMat, 16 * sizeof(Float));
	return mat;
}

QuantumEngine::Vector3 QuantumEngine::Matrix4::operator*(const Vector3& vector)
{
	return Vector3(m_values[0] * vector.x + m_values[1] * vector.y + m_values[2] * vector.z,
		m_values[4] * vector.x + m_values[5] * vector.y + m_values[6] * vector.z,
		m_values[8] * vector.x + m_values[9] * vector.y + m_values[10] * vector.z);
}

QuantumEngine::Matrix4 QuantumEngine::Matrix4::Scale(const Vector3& scale)
{
	return Matrix4{
		scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, scale.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

QuantumEngine::Matrix4 QuantumEngine::Matrix4::Translate(const Vector3& translate)
{
	return Matrix4{
		1.0f, 0.0f, 0.0f, translate.x,
		0.0f, 1.0f, 0.0f, translate.y,
		0.0f, 0.0f, 1.0f, translate.z,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

QuantumEngine::Matrix4 QuantumEngine::Matrix4::Rotate(const Vector3& axis, Float angleDeg)
{
	float cT = cosf(angleDeg * (PI / 180));
	float sT = sinf(angleDeg * (PI / 180));
	Vector3 n = axis.Normalize();
	return Matrix4{
		cT + (1 - cT) * (n.x * n.x), (1 - cT) * n.x * n.y + sT * n.z, (1 - cT) * n.x * n.z - sT * n.y, 0.0f,
		(1 - cT) * n.x * n.y - sT * n.z, cT + (1 - cT) * (n.y * n.y), (1 - cT) * n.y * n.z + sT * n.x, 0.0f,
		(1 - cT) * n.x * n.z + sT * n.y, (1 - cT) * n.y * n.z - sT * n.x, cT + (1 - cT) * (n.z * n.z), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

QuantumEngine::Matrix4 QuantumEngine::Matrix4::PerspectiveProjection(Float near, Float far, Float aspect, Float FOV)
{
	float tanT = 1.0f / tanf(FOV * (PI / 360));
	return Matrix4{
		(tanT / aspect), 0.0f, 0.0f, 0.0f,
		0.0f, tanT, 0.0f, 0.0f,
		0.0f, 0.0f, far / (far - near), (far * near) / (near - far),
		0.0f, 0.0f, 1.0f, 0.0f,
	};
}

QuantumEngine::Matrix4 QuantumEngine::Matrix4::InversePerspectiveProjection(Float near, Float far, Float aspect, Float FOV)
{
	float tanT = 1.0f / tanf(FOV * (PI / 360));
	return Matrix4{
		aspect / tanT, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f / tanT, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, (near - far) / (far * near), 1.0f / near,
	};
}
