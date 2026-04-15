#pragma once
#include "../BasicTypes.h"
#include <string>

namespace QuantumEngine {

	/// <summary>
	/// struct representing a 3D vector
	/// </summary>
	struct Vector3 {
	public: // Constructors

		Vector3();
		Vector3(Float x);
		Vector3(Float x, Float y, Float z);

	public: // Methods

		/// <summary>
		/// returns a normalized copy of this vector
		/// </summary>
		/// <returns></returns>
		Vector3 Normalize() const;

		/// <summary>
		/// returns the magnitude (length) of this vector
		/// </summary>
		/// <returns></returns>
		Float Magnitude() const;

		/// <summary>
		/// returns the square magnitude (length squared) of this vector
		/// </summary>
		/// <returns></returns>
		Float SquareMagnitude() const;

		/// <summary>
		/// returns a string representation of this vector
		/// </summary>
		/// <returns></returns>
		std::string ToString() const;

	public: // Operators

		Vector3 operator-();
		Vector3 operator+(const Vector3& vectorB);
		Vector3 operator+=(const Vector3& vectorB);
		Vector3 operator-(const Vector3& vectorB);
		Vector3 operator-=(const Vector3& vectorB);
		Vector3 operator*(Float fValue);

	public: // static methods

		/// <summary>
		/// returns the dot product of two vectors
		/// </summary>
		/// <param name="vectorA"></param>
		/// <param name="vectorB"></param>
		/// <returns></returns>
		static Float Dot(const Vector3& vectorA, const Vector3& vectorB);

	public: // Fields
		Float x;
		Float y;
		Float z;
	};
}

QuantumEngine::Vector3 operator*(Float fValue, const QuantumEngine::Vector3& vector);