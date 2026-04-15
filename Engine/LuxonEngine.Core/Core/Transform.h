#pragma once
#include "../BasicTypes.h"
#include "Vector3.h"
#include "Matrix4.h"

namespace QuantumEngine {
	class Transform {
	public:
		Transform(const Vector3& position, const Vector3& scale, const Vector3& axir, Float angleDeg);
		Transform();
		inline Vector3 Position() const { return m_position; }
		inline Vector3 Scale() const { return m_scale; }
		inline Vector3 RotationAxis() const { return m_axis; }
		inline Float GetAngle() const { return m_angle; }
		inline Vector3 Forward() const { return m_forward; }
		inline Vector3 Up() const { return m_up; }
		inline Vector3 Right() const { return m_right; }
		inline Matrix4 Matrix() const { return m_matrix; }		
		inline Matrix4 RotateMatrix() const { return Matrix4::Rotate(m_axis, m_angle); }
		void SetPosition(const Vector3& position) {
			m_position = position; 
			UpdateMatrix();
		}
		void SetRotation(const Vector3& axis, Float angleDeg) { 
			m_axis = axis,
			m_angle = angleDeg; 
			UpdateDirections();
			UpdateMatrix();
		}
		void SetScale(const Vector3& scale) { 
			m_scale = scale;
			UpdateMatrix();
		}

		void MoveForward(Float delta);
		void MoveRight(Float delta);
		void RotateAround(const Vector3& axis, Float angleDeg);
		void Translate(const Vector3& delta);
	private:
		void UpdateDirections();
		void UpdateMatrix();
	private:
		Vector3 m_position;
		Vector3 m_scale;
		Vector3 m_axis;
		Float m_angle;
		Vector3 m_forward;
		Vector3 m_right;
		Vector3 m_up;
		Matrix4 m_matrix;
	};
}