#pragma once
#include "Core/Behaviour.h"
#include <Rendering/Material.h>
#include <Core/Vector3.h>

using namespace QuantumEngine;

class MaterialValueModifier : public QuantumEngine::Behaviour
{
public:
	MaterialValueModifier(ref<Rendering::Material>& material, const std::string& fieldName, Float speed, Float minValue, Float maxValue);
	virtual void Update(Float deltaTime) override;
private:
	ref<Rendering::Material> m_material;
	std::string m_fieldName;
	Float m_currentValue;
	Float m_speed;
	Float m_minValue;
	Float m_maxValue;
};