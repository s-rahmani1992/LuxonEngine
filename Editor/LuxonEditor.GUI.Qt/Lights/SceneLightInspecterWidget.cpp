#include "SceneLightInspecterWidget.h"
#include <QHBoxLayout>

SceneLightInspecterWidget::SceneLightInspecterWidget(LuxonEngine::SceneLightData* lightData, QWidget* parent)
	: QWidget(parent), m_lightData(lightData)
{
	auto* rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(4, 4, 4, 4);
	rootLayout->setSpacing(8);

	// ── Directional Lights ────────────────────────────────────────────────

	auto* directionalHeaderRow = new QHBoxLayout();
	m_directionalLightLabel = new QLabel("Directional Lights", this);
	m_addDirectionalButton = new QPushButton("+", this);
	directionalHeaderRow->addWidget(m_directionalLightLabel);
	directionalHeaderRow->addStretch();
	directionalHeaderRow->addWidget(m_addDirectionalButton);
	rootLayout->addLayout(directionalHeaderRow);

	m_directionalPanel = new QWidget();
	m_directionalPanelLayout = new QVBoxLayout(m_directionalPanel);
	m_directionalPanelLayout->setContentsMargins(2, 2, 2, 2);
	m_directionalPanelLayout->setSpacing(4);
	m_directionalPanelLayout->addStretch();

	for (int i = 0; i < static_cast<int>(m_lightData->directionalLights.size()); ++i)
	{
		auto* item = new DirectionalLightItem(&m_lightData->directionalLights[i], i, m_directionalPanel);
		connect(item, &DirectionalLightItem::RemoveRequested, this, &SceneLightInspecterWidget::OnDirectionalLightRemoveRequested);
		m_directionalPanelLayout->insertWidget(m_directionalPanelLayout->count() - 1, item);
		m_directionalItems.append(item);
	}

	m_directionalScrollArea = new QScrollArea(this);
	m_directionalScrollArea->setWidget(m_directionalPanel);
	m_directionalScrollArea->setWidgetResizable(true);
	rootLayout->addWidget(m_directionalScrollArea);

	// ── Point Lights ──────────────────────────────────────────────────────

	auto* pointHeaderRow = new QHBoxLayout();
	m_pointLightLabel = new QLabel("Point Lights", this);
	m_addPointButton = new QPushButton("+", this);
	pointHeaderRow->addWidget(m_pointLightLabel);
	pointHeaderRow->addStretch();
	pointHeaderRow->addWidget(m_addPointButton);
	rootLayout->addLayout(pointHeaderRow);

	m_pointPanel = new QWidget();
	m_pointPanelLayout = new QVBoxLayout(m_pointPanel);
	m_pointPanelLayout->setContentsMargins(2, 2, 2, 2);
	m_pointPanelLayout->setSpacing(4);
	m_pointPanelLayout->addStretch();

	for (int i = 0; i < static_cast<int>(m_lightData->pointLights.size()); ++i)
	{
		auto* item = new PointLightItem(&m_lightData->pointLights[i], i, m_pointPanel);
		connect(item, &PointLightItem::RemoveRequested, this, &SceneLightInspecterWidget::OnPointLightRemoveRequested);
		m_pointPanelLayout->insertWidget(m_pointPanelLayout->count() - 1, item);
		m_pointItems.append(item);
	}

	m_pointScrollArea = new QScrollArea(this);
	m_pointScrollArea->setWidget(m_pointPanel);
	m_pointScrollArea->setWidgetResizable(true);
	rootLayout->addWidget(m_pointScrollArea);

	setLayout(rootLayout);

	connect(m_addDirectionalButton, &QPushButton::clicked, this, &SceneLightInspecterWidget::OnAddDirectionalLight);
	connect(m_addPointButton, &QPushButton::clicked, this, &SceneLightInspecterWidget::OnAddPointLight);
}

SceneLightInspecterWidget::~SceneLightInspecterWidget()
{
}

void SceneLightInspecterWidget::OnAddDirectionalLight()
{
	auto& lights = m_lightData->directionalLights;

	// push_back may reallocate — update all existing items first after the push
	lights.push_back(LuxonEngine::DirectionalLight{});

	// Refresh all existing item pointers in case of reallocation
	for (int i = 0; i < m_directionalItems.size(); ++i)
		m_directionalItems[i]->SetLight(&lights[i], i);

	// Add widget for the new light
	const int newIndex = static_cast<int>(lights.size()) - 1;
	auto* item = new DirectionalLightItem(&lights[newIndex], newIndex, m_directionalPanel);
	connect(item, &DirectionalLightItem::RemoveRequested, this, &SceneLightInspecterWidget::OnDirectionalLightRemoveRequested);
	m_directionalPanelLayout->insertWidget(m_directionalPanelLayout->count() - 1, item);
	m_directionalItems.append(item);
}

void SceneLightInspecterWidget::OnAddPointLight()
{
	auto& lights = m_lightData->pointLights;

	// push_back may reallocate — update all existing items first after the push
	lights.push_back(LuxonEngine::PointLight{});

	// Refresh all existing item pointers in case of reallocation
	for (int i = 0; i < m_pointItems.size(); ++i)
		m_pointItems[i]->SetLight(&lights[i], i);

	// Add widget for the new light
	const int newIndex = static_cast<int>(lights.size()) - 1;
	auto* item = new PointLightItem(&lights[newIndex], newIndex, m_pointPanel);
	connect(item, &PointLightItem::RemoveRequested, this, &SceneLightInspecterWidget::OnPointLightRemoveRequested);
	m_pointPanelLayout->insertWidget(m_pointPanelLayout->count() - 1, item);
	m_pointItems.append(item);
}

void SceneLightInspecterWidget::OnDirectionalLightRemoveRequested(LuxonEngine::DirectionalLight* light)
{
	auto& lights = m_lightData->directionalLights;
	const auto dataIt = std::find_if(lights.begin(), lights.end(), [light](const LuxonEngine::DirectionalLight& l) {
		return &l == light;
		});

	if (dataIt == lights.end())
		return;

	const int index = static_cast<int>(std::distance(lights.begin(), dataIt));

	DirectionalLightItem* item = m_directionalItems[index];
	m_directionalItems.removeAt(index);
	item->deleteLater();

	lights.erase(dataIt);

	for (int i = 0; i < m_directionalItems.size(); ++i)
		m_directionalItems[i]->SetLight(&lights[i], i);
}

void SceneLightInspecterWidget::OnPointLightRemoveRequested(LuxonEngine::PointLight* light)
{
	auto& lights = m_lightData->pointLights;
	const auto dataIt = std::find_if(lights.begin(), lights.end(), [light](const LuxonEngine::PointLight& l) {
		return &l == light;
		});

	if (dataIt == lights.end())
		return;

	const int index = static_cast<int>(std::distance(lights.begin(), dataIt));

	PointLightItem* item = m_pointItems[index];
	m_pointItems.removeAt(index);
	item->deleteLater();

	lights.erase(dataIt);

	for (int i = 0; i < m_pointItems.size(); ++i)
		m_pointItems[i]->SetLight(&lights[i], i);
}