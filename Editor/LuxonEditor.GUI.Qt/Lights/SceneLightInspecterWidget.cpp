#include "SceneLightInspecterWidget.h"

SceneLightInspecterWidget::SceneLightInspecterWidget(LuxonEngine::SceneLightData* lightData, QWidget* parent)
	: QWidget(parent), m_lightData(lightData)
{
	auto* rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(4, 4, 4, 4);
	rootLayout->setSpacing(8);

	// ── Directional Lights ────────────────────────────────────────────────

	m_directionalLightLabel = new QLabel("Directional Lights", this);
	rootLayout->addWidget(m_directionalLightLabel);

	m_directionalPanel = new QWidget();
	m_directionalPanelLayout = new QVBoxLayout(m_directionalPanel);
	m_directionalPanelLayout->setContentsMargins(2, 2, 2, 2);
	m_directionalPanelLayout->setSpacing(4);
	m_directionalPanelLayout->addStretch();

	for (int i = 0; i < static_cast<int>(m_lightData->directionalLights.size()); ++i)
	{
		auto* item = new DirectionalLightItem(&m_lightData->directionalLights[i], i, m_directionalPanel);
		m_directionalPanelLayout->insertWidget(m_directionalPanelLayout->count() - 1, item);
	}

	m_directionalScrollArea = new QScrollArea(this);
	m_directionalScrollArea->setWidget(m_directionalPanel);
	m_directionalScrollArea->setWidgetResizable(true);
	rootLayout->addWidget(m_directionalScrollArea);

	// ── Point Lights ──────────────────────────────────────────────────────

	m_pointLightLabel = new QLabel("Point Lights", this);
	rootLayout->addWidget(m_pointLightLabel);

	m_pointPanel = new QWidget();
	m_pointPanelLayout = new QVBoxLayout(m_pointPanel);
	m_pointPanelLayout->setContentsMargins(2, 2, 2, 2);
	m_pointPanelLayout->setSpacing(4);
	m_pointPanelLayout->addStretch();

	for (int i = 0; i < static_cast<int>(m_lightData->pointLights.size()); ++i)
	{
		auto* item = new PointLightItem(&m_lightData->pointLights[i], i, m_pointPanel);
		m_pointPanelLayout->insertWidget(m_pointPanelLayout->count() - 1, item);
	}

	m_pointScrollArea = new QScrollArea(this);
	m_pointScrollArea->setWidget(m_pointPanel);
	m_pointScrollArea->setWidgetResizable(true);
	rootLayout->addWidget(m_pointScrollArea);

	setLayout(rootLayout);
}

SceneLightInspecterWidget::~SceneLightInspecterWidget()
{
}