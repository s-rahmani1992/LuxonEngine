#pragma once

#include "Core/Behaviour.h"
#include <Core/Transform.h>
#include <Core/Vector3.h>
#include <Rendering/Material.h>

using namespace QuantumEngine;

class TextureSwitcher : public QuantumEngine::Behaviour
{
public:
	TextureSwitcher(ref<Rendering::Material>& material, const std::string& fieldName, const std::vector<ref<Texture2D>>& textures);
	virtual void Update(Float deltaTime) override;
private:
	ref<Rendering::Material> m_material;
	std::string m_fieldName;
	std::vector<ref<Texture2D>> m_textures;
	int m_index = 0;
	bool m_keyPressed = false;
};
