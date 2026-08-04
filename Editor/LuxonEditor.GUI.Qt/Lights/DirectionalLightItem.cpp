#include "DirectionalLightItem.h"
#include <QHBoxLayout>

DirectionalLightItem::DirectionalLightItem(LuxonEngine::DirectionalLight* light, int index, QWidget* parent)
	: QWidget(parent), m_light(light)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	// Title row
	auto* titleRow = new QHBoxLayout();
	m_titleLabel = new QLabel(QString("Directional Light %1").arg(index), this);
	m_removeButton = new QPushButton("Remove", this);
	titleRow->addWidget(m_titleLabel);
	titleRow->addStretch();
	titleRow->addWidget(m_removeButton);
	layout->addLayout(titleRow);

	// Color
	m_colorField = new QColorField(this, "Color");
	m_colorField->SetColor(light->color);
	layout->addWidget(m_colorField);

	// Direction
	m_directionField = new QVector3Field(this);
	m_directionField->setLabelText("Direction");
	m_directionField->setValue(light->direction);
	layout->addWidget(m_directionField);

	// Intensity
	m_intensityField = new QFloatField(this);
	m_intensityField->setLabelText("Intensity");
	m_intensityField->setValue(light->intensity);
	layout->addWidget(m_intensityField);

	// Separator
	m_separator = new QFrame(this);
	m_separator->setFrameShape(QFrame::HLine);
	m_separator->setFrameShadow(QFrame::Sunken);
	layout->addWidget(m_separator);

	setLayout(layout);

	connect(m_colorField, &QColorField::ValueChanged, this, &DirectionalLightItem::OnColorChanged);
	connect(m_directionField, &QVector3Field::ValueChanged, this, &DirectionalLightItem::OnDirectionChanged);
	connect(m_intensityField, &QFloatField::ValueChanged, this, &DirectionalLightItem::OnIntensityChanged);
	connect(m_removeButton, &QPushButton::clicked, this, [this]() {
		emit RemoveRequested(m_light);
		});
}

DirectionalLightItem::~DirectionalLightItem()
{
}

void DirectionalLightItem::SetLight(LuxonEngine::DirectionalLight* light, int index)
{
	m_light = light;
	m_titleLabel->setText(QString("Directional Light %1").arg(index));
}

void DirectionalLightItem::OnColorChanged(LuxonEngine::Color newColor)
{
	m_light->color = newColor;
}

void DirectionalLightItem::OnDirectionChanged(LuxonEngine::Vector3 newDirection)
{
	m_light->direction = newDirection;
}

void DirectionalLightItem::OnIntensityChanged(float newValue)
{
	m_light->intensity = newValue;
}