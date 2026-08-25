#pragma once
#include <EngineAPI.h>

using namespace LuxonEngine;

class __declspec(dllexport) EntityPositionController : public LuxonEngine::Behaviour
{
public:
	EntityPositionController();
	EntityPositionController(ref<LuxonEngine::Transform>& transform, Float speed);

	virtual void Update(Float deltaTime) override;

	inline void SetTransform(ref<LuxonEngine::Transform>& transform) { m_transform = transform; }
	inline ref<LuxonEngine::Transform> GetTransform() const { return m_transform; }

	inline void SetSpeed(Float speed) { m_speed = speed; }
	inline Float GetSpeed() const { return m_speed; }

private:
	ref<LuxonEngine::Transform> m_transform;
	Float m_speed;
};