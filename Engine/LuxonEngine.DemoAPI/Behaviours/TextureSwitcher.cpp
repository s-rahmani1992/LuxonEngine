#include "TextureSwitcher.h"
#include "Platform/CommonWin.h"

TextureSwitcher::TextureSwitcher(ref<Rendering::Material>& material, const std::string& fieldName, const std::vector<ref<Texture2D>>& textures)
	:m_material(material), m_fieldName(fieldName), m_textures(textures)
{
	m_material->SetTexture2D(m_fieldName, m_textures[m_index]);
}

void TextureSwitcher::Update(Float deltaTime)
{
	if ((GetKeyState(VK_SPACE) & 0x80) && !m_keyPressed) {
		m_keyPressed = true;
		m_index = (m_index + 1) % m_textures.size();
		m_material->SetTexture2D(m_fieldName, m_textures[m_index]);
	}

	else if (m_keyPressed && ((GetKeyState(VK_SPACE) & 0x80) == 0))
		m_keyPressed = false;
}
