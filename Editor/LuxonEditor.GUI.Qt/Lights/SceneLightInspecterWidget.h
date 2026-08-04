#pragma once

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QVector>
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
	void OnAddDirectionalLight();
	void OnAddPointLight();
	void OnDirectionalLightRemoveRequested(LuxonEngine::DirectionalLight* light);
	void OnPointLightRemoveRequested(LuxonEngine::PointLight* light);

	LuxonEngine::SceneLightData* m_lightData;

	// Directional Lights section
	QLabel* m_directionalLightLabel;
	QPushButton* m_addDirectionalButton;
	QScrollArea* m_directionalScrollArea;
	QWidget* m_directionalPanel;
	QVBoxLayout* m_directionalPanelLayout;
	QVector<DirectionalLightItem*> m_directionalItems;

	// Point Lights section
	QLabel* m_pointLightLabel;
	QPushButton* m_addPointButton;
	QScrollArea* m_pointScrollArea;
	QWidget* m_pointPanel;
	QVBoxLayout* m_pointPanelLayout;
	QVector<PointLightItem*> m_pointItems;
};