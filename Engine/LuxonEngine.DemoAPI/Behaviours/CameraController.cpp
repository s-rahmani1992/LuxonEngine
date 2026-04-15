#include "CameraController.h"
#include "Platform/CommonWin.h"
#include "Core/Transform.h"
#include <string>

CameraController::CameraController(ref<QuantumEngine::Camera>& camera)
	:m_camera(camera), m_moveSpeed(2.0f), m_rotateSpeed(10)
{
	POINT mousePos;
	GetCursorPos(&mousePos);
	xPos = mousePos.x;
	yPos = mousePos.y;
}

void CameraController::Update(Float deltaTime)
{
	if (GetKeyState('P') & 0x80) {
		auto pos = m_camera->GetTransform()->Position();
		auto axis = m_camera->GetTransform()->RotationAxis();
		auto angle = m_camera->GetTransform()->GetAngle();
		OutputDebugStringA((pos.ToString() + "\n").c_str());
		OutputDebugStringA((axis.ToString() + "\n").c_str());
		OutputDebugStringA((std::to_string(angle) + "\n").c_str());
	}

	// TODO Replace it with event-based input system
	if ((GetKeyState(VK_RBUTTON) & 0x80) == 0) {
		POINT mousePos;
		GetCursorPos(&mousePos);
		xPos = mousePos.x;
		yPos = mousePos.y;
		return;
	}

	POINT mousePos;
	GetCursorPos(&mousePos);
	Float angle_x = m_rotateSpeed * deltaTime * (mousePos.x - xPos);
	Float angle_y = m_rotateSpeed * deltaTime * (mousePos.y - yPos);
	m_camera->GetTransform()->RotateAround(m_camera->GetTransform()->Up(), -angle_x);
	m_camera->GetTransform()->RotateAround(m_camera->GetTransform()->Right(), -angle_y);
	xPos = mousePos.x;
	yPos = mousePos.y;

	if (GetKeyState('W') & 0x80) {
		m_camera->GetTransform()->MoveForward(m_moveSpeed * deltaTime);
	}

	if (GetKeyState('S') & 0x80) {
		m_camera->GetTransform()->MoveForward(-m_moveSpeed * deltaTime);
	}

	if (GetKeyState('A') & 0x80) {
		m_camera->GetTransform()->MoveRight(-m_moveSpeed * deltaTime);
	}

	if (GetKeyState('D') & 0x80) {
		m_camera->GetTransform()->MoveRight(m_moveSpeed * deltaTime);
	}

	
}
