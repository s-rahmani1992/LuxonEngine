#include "EntityPositionController.h"
#include "Platform/CommonWin.h"

EntityPositionController::EntityPositionController(ref<QuantumEngine::Transform>& transform, float speed)
	:m_transform(transform), m_speed(speed)
{
}

void EntityPositionController::Update(Float deltaTime)
{
	if (GetKeyState(VK_UP) & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(0.0f, 0.0f, -1.0f));
	}

	if (GetKeyState(VK_DOWN) & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(0.0f, 0.0f, 1.0f));
	}

	if (GetKeyState(VK_LEFT) & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(1.0f, 0.0f, 0.0f));
	}

	if (GetKeyState(VK_RIGHT) & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(-1.0f, 0.0f, 0.0f));
	}
}
