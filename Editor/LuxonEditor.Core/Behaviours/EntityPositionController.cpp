#include "EntityPositionController.h"
#include "Platform/CommonWin.h"

EntityPositionController::EntityPositionController(ref<LuxonEngine::Transform>& transform, float speed)
	:m_transform(transform), m_speed(speed)
{
}

EntityPositionController::EntityPositionController()
	:m_transform(nullptr), m_speed(10.0f)
{
}

void EntityPositionController::Update(Float deltaTime)
{
	if (GetKeyState(VK_UP) & 0x80) {
		m_transform->MoveForward(deltaTime * m_speed);
	}

	if (GetKeyState(VK_DOWN) & 0x80) {
		m_transform->MoveForward(-(deltaTime * m_speed));
	}

	if (GetKeyState(VK_LEFT) & 0x80) {
		m_transform->MoveRight(-(deltaTime * m_speed));
	}

	if (GetKeyState(VK_RIGHT) & 0x80) {
		m_transform->MoveRight(deltaTime * m_speed);
	}
}
