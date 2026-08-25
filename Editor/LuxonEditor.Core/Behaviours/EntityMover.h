#pragma once

#include <EngineAPI.h>

using namespace LuxonEngine;

class __declspec(dllexport) EntityMover : public LuxonEngine::Behaviour
{
public:
	EntityMover();
	EntityMover(ref<LuxonEngine::Transform>& transform, Vector3 point1, Vector3 point2, float start, float speed);
	virtual void Start() override;
	virtual void Update(Float deltaTime) override;

	inline ref<LuxonEngine::Transform> GetTransform() const { return m_transform; }
	inline void SetTransform(ref<LuxonEngine::Transform>& transform) { m_transform = transform; }

	inline void SetPoint1(const Vector3& point1) { m_points[0] = point1; }
	inline Vector3 GetPoint1() const { return m_points[0]; }

	inline void SetPoint2(const Vector3& point2) { m_points[1] = point2; }
	inline Vector3 GetPoint2() const { return m_points[1]; }

	inline void SetSpeed(const Float& speed) { m_speedMagnitude = speed; }
	inline Float GetSpeed() const { return m_speedMagnitude; }

	inline void SetStart(float start) { m_start = start; }
	inline float GetStart() const { return m_start; }
private:
	ref<LuxonEngine::Transform> m_transform;
	Vector3 m_points[2];
	Vector3 m_currentPosition;
	UInt8 m_currentTargetIndex;
	Float m_currentPoint;
	Vector3 m_speed;
	Vector3 m_currentTarget;
	Float m_start;
	Float m_speedMagnitude;
};

