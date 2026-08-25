#pragma once
#include <EngineAPI.h>

using namespace LuxonEngine;

class __declspec(dllexport) EntityRotator : public LuxonEngine::Behaviour
{
public:
	EntityRotator();
	EntityRotator(ref<LuxonEngine::Transform>& transform, float speed, Vector3 axis);

	virtual void Start() override;
	virtual void Update(Float deltaTime) override;

	inline void SetTransform(ref<LuxonEngine::Transform>& transform) { m_transform = transform; }
	inline ref<LuxonEngine::Transform> GetTransform() const { return m_transform; }

	inline void SetSpeed(Float speed) { m_speed = speed; }
	inline Float GetSpeed() const { return m_speed; }

	inline void SetAxis(const Vector3& axis) { m_axis = axis; }
	inline Vector3 GetAxis() const { return m_axis; }
private:
	ref<LuxonEngine::Transform> m_transform;
	Float m_currentAngle;
	Float m_speed;
	Vector3 m_axis;
};
