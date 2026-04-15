#include "MaterialValueModifier.h"
#include "Platform/CommonWin.h"

MaterialValueModifier::MaterialValueModifier(ref<Rendering::Material>& material, const std::string& fieldName, Float speed, Float minValue, Float maxValue)
	:m_material(material), m_fieldName(fieldName), m_speed(speed), m_minValue(minValue), m_maxValue(maxValue),
	m_currentValue(material->GetValue(fieldName, 0.0f))
{
}

void MaterialValueModifier::Update(Float deltaTime)
{
	if (GetKeyState(VK_OEM_PLUS) & 0x80) {
		m_currentValue += m_speed * deltaTime;

		if (m_currentValue > m_maxValue)
			m_currentValue = m_maxValue;

		m_material->SetValue(m_fieldName, m_currentValue);
	}

	if (GetKeyState(VK_OEM_MINUS) & 0x80) {
		m_currentValue -= m_speed * deltaTime;

		if (m_currentValue < m_minValue)
			m_currentValue = m_minValue;

		m_material->SetValue(m_fieldName, m_currentValue);
	}
}
