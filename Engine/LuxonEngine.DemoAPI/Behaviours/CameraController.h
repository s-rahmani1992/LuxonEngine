#pragma once
#include "Core/Camera/Camera.h"
#include "Core/Behaviour.h"

class CameraController : public QuantumEngine::Behaviour
{
public:
	CameraController(ref<QuantumEngine::Camera>& camera);
	virtual void Update(Float deltaTime) override;
private:
	ref<QuantumEngine::Camera> m_camera;
	Float m_moveSpeed;
	Float m_rotateSpeed;
	Float xPos;
	Float yPos;
};

