#pragma once
#include <initializer_list>
#include "../BasicTypes.h"

namespace QuantumEngine {
	struct Vector3;

	struct Matrix4 {
	public:
		Matrix4(const std::initializer_list<Float>& values);
		Matrix4();
		Matrix4 operator*(const Matrix4& matrixB);
		Vector3 operator*(const Vector3& matrixB);
		static Matrix4 Scale(const Vector3& scale);
		static Matrix4 Translate(const Vector3& translate);
		static Matrix4 Rotate(const Vector3& axis, Float angleDeg);
		static Matrix4 PerspectiveProjection(Float near, Float far, Float acpect, Float FOV);
		static Matrix4 InversePerspectiveProjection(Float near, Float far, Float acpect, Float FOV);
		Float operator()(UInt8 x, UInt8 y) { return m_values[4 * x + y]; }
		void SetValue(UInt8 x, UInt8 y, Float value) { m_values[4 * x + y] = value; }
	private:
		Float m_values[16];
	};
}