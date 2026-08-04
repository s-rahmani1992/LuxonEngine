#include "DirectionalLightItem.h"

DirectionalLightItem::DirectionalLightItem(LuxonEngine::DirectionalLight* light, int index, QWidget* parent)
	: QWidget(parent), m_light(light)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	// Title
	m_titleLabel = new QLabel(QString("Directional Light %1").arg(index), this);
	layout->addWidget(m_titleLabel);

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
}

DirectionalLightItem::~DirectionalLightItem()
{
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