#pragma once

#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <EngineAPI.h>
#include "../Texture/QColorField.h"
#include <Widgets/QVector3Field.h>
#include <Widgets/QFloatField.h>

class PointLightItem : public QWidget
{
	Q_OBJECT

public:
	PointLightItem(LuxonEngine::PointLight* light, int index, QWidget* parent = nullptr);
	~PointLightItem();

private:
	void OnColorChanged(LuxonEngine::Color newColor);
	void OnPositionChanged(LuxonEngine::Vector3 newPosition);
	void OnIntensityChanged(float newValue);
	void OnRadiusChanged(float newValue);
	void OnAttenuationC0Changed(float newValue);
	void OnAttenuationC1Changed(float newValue);
	void OnAttenuationC2Changed(float newValue);

	LuxonEngine::PointLight* m_light;

	QLabel* m_titleLabel;
	QColorField* m_colorField;
	QVector3Field* m_positionField;
	QFloatField* m_intensityField;
	QFloatField* m_radiusField;
	QFloatField* m_attC0Field;
	QFloatField* m_attC1Field;
	QFloatField* m_attC2Field;
	QFrame* m_separator;
};