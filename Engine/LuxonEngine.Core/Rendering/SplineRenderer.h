#pragma once

#include "../BasicTypes.h"
#include "Renderer.h"
#include <vector>
#include "../Core/BezierCurve.h"

namespace LuxonEngine::Rendering {
	class Material;

	class SplineRenderer : public Renderer
	{
	public: // Constructors

		SplineRenderer(const ref<Material>& material, const std::vector<Vector3>& points, const float width, const int segments, const float tileFactor)
			: Renderer(material), m_curve(points[0], points[1], points[2]),
			m_width(width), m_segments(segments), m_tileFactor(tileFactor) {
		}

	public: // Methods

		/// <summary>
		/// returns the width of the spline mesh
		/// </summary>
		inline Float GetWidth() const { return m_width; }

		/// <summary>
		/// sets the width of the spline mesh
		/// </summary>
		inline void SetWidth(float width) { m_width = width; }

		/// <summary>
		/// returns the number of segments used to render the spline mesh
		/// </summary>
		inline int GetSegments() const { return m_segments; }

		/// <summary>
		/// sets the number of segments used to render the spline mesh
		/// </summary>
		inline void SetSegments(int segments) { m_segments = segments; }

		inline float GetTileFactor() const { return m_tileFactor; }

		inline void SetTileFactor(float tileFactor) { m_tileFactor = tileFactor; }

		/// <summary>
		/// Gets the bezier curve used by this spline renderer
		/// </summary>
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
		float m_tileFactor = 1.0f;
		Core::BezierCurve m_curve;
	};
}