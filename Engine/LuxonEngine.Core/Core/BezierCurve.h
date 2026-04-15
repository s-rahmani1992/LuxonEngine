#pragma 
#include "Vector2.h"
#include "Vector3.h"

namespace QuantumEngine::Core {
class BezierCurve{
    public:
        BezierCurve(const Vector3& point1, const Vector3& point2, const Vector3& point3);
        
        /// <summary>
		/// fills the position and tangent at t value on the curve
        /// </summary>
        /// <param name="t"></param>
        /// <param name="position"></param>
        /// <param name="tangent"></param>
        void Interpolate(Float t, Vector3* position, Vector3* tangent) const;

        /// <summary>
		/// returns the length of the curve from 0 to t value
        /// </summary>
        /// <param name="t"></param>
        /// <returns></returns>
        Float InterpolateLength(Float t) const;

    private:
        Float Integral(Float t) const;

    public:
        Vector3 m_point1;
        Vector3 m_point2;
        Vector3 m_point3;
    };
}