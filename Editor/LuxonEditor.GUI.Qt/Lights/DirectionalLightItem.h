#pragma once

#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <EngineAPI.h>
#include "../Texture/QColorField.h"
#include "../../LuxonEditor.GUI.Qt.Widgets/Widgets/QVector3Field.h"
#include "../../LuxonEditor.GUI.Qt.Widgets/Widgets/QFloatField.h"

class DirectionalLightItem : public QWidget
{
	Q_OBJECT

public:
	DirectionalLightItem(LuxonEngine::DirectionalLight* light, int index, QWidget* parent = nullptr);
	~DirectionalLightItem();

private:
	void OnColorChanged(LuxonEngine::Color newColor);
	void OnDirectionChanged(LuxonEngine::Vector3 newDirection);
	void OnIntensityChanged(float newValue);

	LuxonEngine::DirectionalLight* m_light;

	QLabel* m_titleLabel;
	QColorField* m_colorField;
	QVector3Field* m_directionField;
	QFloatField* m_intensityField;
	QFrame* m_separator;
};