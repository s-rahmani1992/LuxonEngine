#pragma once
#include <EngineAPI.h>

using namespace LuxonEngine;

class __declspec(dllexport) MaterialValueModifier : public LuxonEngine::Behaviour
{
public:
	MaterialValueModifier();
	MaterialValueModifier(ref<Rendering::Material>& material, const std::string& fieldName, Float speed, Float minValue, Float maxValue);

	inline ref<Rendering::Material> GetMaterial() const { return m_material; }
	inline void SetMaterial(ref<Rendering::Material>& material) { m_material = material; }

	inline const std::string& GetFieldName() const { return m_fieldName; }
	inline void SetFieldName(const std::string& fieldName) { m_fieldName = fieldName; }

	inline Float GetSpeed() const { return m_speed; }
	inline void SetSpeed(Float speed) { m_speed = speed; }	
	
	inline Float GetMinValue() const { return m_minValue; }
	inline void SetMinValue(Float minValue) { m_minValue = minValue; }

	inline Float GetMaxValue() const { return m_maxValue; }
	inline void SetMaxValue(Float maxValue) { m_maxValue = maxValue; }
	
	virtual void Start() override;
	virtual void Update(Float deltaTime) override;
private:
	ref<Rendering::Material> m_material;
	std::string m_fieldName;
	Float m_currentValue;
	Float m_speed;
	Float m_minValue;
	Float m_maxValue;
};