#pragma once
#include <EngineAPI.h>

class __declspec(dllexport) BasicCameraNavigator : public LuxonEngine::Behaviour
{
public:
	BasicCameraNavigator();
	BasicCameraNavigator(float forwardMoveSpeed, float sideMoveSpeed, float rotateSpeed);
	virtual void Update(Float deltaTime) override;
	virtual void Start() override;

	inline void SetForwardMoveSpeed(Float speed) { m_forwardMoveSpeed = speed; }
	Float GetForwardMoveSpeed() const { return m_forwardMoveSpeed; }

	inline void SetSideMoveSpeed(Float speed) { m_sideMoveSpeed = speed; }
	Float GetSideMoveSpeed() const { return m_sideMoveSpeed; }

	inline void SetRotateSpeed(Float speed) { m_rotateSpeed = speed; }
	Float GetRotateSpeed() const { return m_rotateSpeed; }
private:
	ref<LuxonEngine::Camera> m_camera;
	ref<LuxonEngine::Transform> m_transform;
	Float m_forwardMoveSpeed;
	Float m_sideMoveSpeed;
	Float m_rotateSpeed;
	Float xPos;
	Float yPos;
};

