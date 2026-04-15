#pragma once

#include "../BasicTypes.h"
#include "Renderer.h"
#include <vector>
#include "../Core/BezierCurve.h"

namespace QuantumEngine::Rendering {
	class Material;

	class SplineRenderer : public Renderer
	{
	public: // Constructors

		SplineRenderer(const ref<Material>& material, const std::vector<Vector3>& points, const float width, const int segments)
			: Renderer(material), m_curve(points[0], points[1], points[2]),
			m_width(width), m_segments(segments) { }
	
	public: // Methods

		/// <summary>
		/// returns the width of the spline mesh
		/// </summary>
		/// <returns></returns>
		inline Float GetWidth() const { return m_width; }

		/// <summary>
		/// returns the number of segments used to create render the spline mesh
		/// </summary>
		/// <returns></returns>
		inline int GetSegments() const { return m_segments; }

		/// <summary>
		/// Gets the bezier curve used by this spline renderer
		/// </summary>
		/// <returns></returns>
		inline Core::BezierCurve& GetCurve() { return m_curve; }

		inline void SetDirty() { m_isDirty = true; }

		inline bool IsDirty()
		{ 
			bool dirty = m_isDirty;
			m_isDirty = false;
			return dirty; 
		}

	private: // Fields
		float m_width;
		int m_segments;
		bool m_isDirty = true;
		Core::BezierCurve m_curve;
	};
}