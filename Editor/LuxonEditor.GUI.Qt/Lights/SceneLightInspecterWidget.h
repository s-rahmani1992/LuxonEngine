#pragma once

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <EngineAPI.h>
#include "PointLightItem.h"
#include "DirectionalLightItem.h"

class SceneLightInspecterWidget : public QWidget
{
	Q_OBJECT

public:
	SceneLightInspecterWidget(LuxonEngine::SceneLightData* lightData, QWidget* parent = nullptr);
	~SceneLightInspecterWidget();

private:
	LuxonEngine::SceneLightData* m_lightData;

	// Directional Lights section
	QLabel* m_directionalLightLabel;
	QScrollArea* m_directionalScrollArea;
	QWidget* m_directionalPanel;
	QVBoxLayout* m_directionalPanelLayout;

	// Point Lights section
	QLabel* m_pointLightLabel;
	QScrollArea* m_pointScrollArea;
	QWidget* m_pointPanel;
	QVBoxLayout* m_pointPanelLayout;
};