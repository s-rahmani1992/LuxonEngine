#include "BasicCameraNavigator.h"
#include "../Core/EngineApplication.h"
#include "../Core/EngineSceneManager.h"
#include <string>

BasicCameraNavigator::BasicCameraNavigator()
	:BasicCameraNavigator(1.0f, 1.0f, 5.0f)
{
}

BasicCameraNavigator::BasicCameraNavigator(float forwardMoveSpeed, float sideMoveSpeed, float rotateSpeed)
	:m_forwardMoveSpeed(forwardMoveSpeed), m_sideMoveSpeed(sideMoveSpeed), m_rotateSpeed(rotateSpeed)
{
	m_camera = LuxonEditor::EngineApplication::GetSceneManager()->GetCurrentScene()->mainCamera;
	m_transform = m_camera->GetTransform();
}

void BasicCameraNavigator::Update(Float deltaTime)
{
	 //TODO Replace it with event-based input system
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
	m_transform->RotateAround(m_transform->Up(), -angle_x);
	m_transform->RotateAround(m_transform->Right(), -angle_y);
	xPos = mousePos.x;
	yPos = mousePos.y;

	if (GetKeyState('W') & 0x80) {
		m_transform->MoveForward(m_forwardMoveSpeed * deltaTime);
	}

	if (GetKeyState('S') & 0x80) {
		m_transform->MoveForward(-m_forwardMoveSpeed * deltaTime);
	}

	if (GetKeyState('A') & 0x80) {
		m_transform->MoveRight(-m_sideMoveSpeed * deltaTime);
	}

	if (GetKeyState('D') & 0x80) {
		m_transform->MoveRight(m_sideMoveSpeed * deltaTime);
	}
}

void BasicCameraNavigator::Start()
{
	POINT mousePos;
	GetCursorPos(&mousePos);
	xPos = mousePos.x;
	yPos = mousePos.y;
}
