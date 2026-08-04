#include "PointLightItem.h"

PointLightItem::PointLightItem(LuxonEngine::PointLight* light, int index, QWidget* parent)
	: QWidget(parent), m_light(light)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	// Title
	m_titleLabel = new QLabel(QString("Point Light %1").arg(index), this);
	layout->addWidget(m_titleLabel);

	// Color
	m_colorField = new QColorField(this, "Color");
	m_colorField->SetColor(light->color);
	layout->addWidget(m_colorField);

	// Position
	m_positionField = new QVector3Field(this);
	m_positionField->setLabelText("Position");
	m_positionField->setValue(light->position);
	layout->addWidget(m_positionField);

	// Intensity
	m_intensityField = new QFloatField(this);
	m_intensityField->setLabelText("Intensity");
	m_intensityField->setValue(light->intensity);
	layout->addWidget(m_intensityField);

	// Radius
	m_radiusField = new QFloatField(this);
	m_radiusField->setLabelText("Radius");
	m_radiusField->setValue(light->radius);
	layout->addWidget(m_radiusField);

	// Attenuation
	m_attC0Field = new QFloatField(this);
	m_attC0Field->setLabelText("Attenuation C0");
	m_attC0Field->setValue(light->attenuation.c0);
	layout->addWidget(m_attC0Field);

	m_attC1Field = new QFloatField(this);
	m_attC1Field->setLabelText("Attenuation C1");
	m_attC1Field->setValue(light->attenuation.c1);
	layout->addWidget(m_attC1Field);

	m_attC2Field = new QFloatField(this);
	m_attC2Field->setLabelText("Attenuation C2");
	m_attC2Field->setValue(light->attenuation.c2);
	layout->addWidget(m_attC2Field);

	// Separator
	m_separator = new QFrame(this);
	m_separator->setFrameShape(QFrame::HLine);
	m_separator->setFrameShadow(QFrame::Sunken);
	layout->addWidget(m_separator);

	setLayout(layout);

	connect(m_colorField, &QColorField::ValueChanged, this, &PointLightItem::OnColorChanged);
	connect(m_positionField, &QVector3Field::ValueChanged, this, &PointLightItem::OnPositionChanged);
	connect(m_intensityField, &QFloatField::ValueChanged, this, &PointLightItem::OnIntensityChanged);
	connect(m_radiusField, &QFloatField::ValueChanged, this, &PointLightItem::OnRadiusChanged);
	connect(m_attC0Field, &QFloatField::ValueChanged, this, &PointLightItem::OnAttenuationC0Changed);
	connect(m_attC1Field, &QFloatField::ValueChanged, this, &PointLightItem::OnAttenuationC1Changed);
	connect(m_attC2Field, &QFloatField::ValueChanged, this, &PointLightItem::OnAttenuationC2Changed);
}

PointLightItem::~PointLightItem()
{
}

void PointLightItem::OnColorChanged(LuxonEngine::Color newColor)
{
	m_light->color = newColor;
}

void PointLightItem::OnPositionChanged(LuxonEngine::Vector3 newPosition)
{
	m_light->position = newPosition;
}

void PointLightItem::OnIntensityChanged(float newValue)
{
	m_light->intensity = newValue;
}

void PointLightItem::OnRadiusChanged(float newValue)
{
	m_light->radius = newValue;
}

void PointLightItem::OnAttenuationC0Changed(float newValue)
{
	m_light->attenuation.c0 = newValue;
}

void PointLightItem::OnAttenuationC1Changed(float newValue)
{
	m_light->attenuation.c1 = newValue;
}

void PointLightItem::OnAttenuationC2Changed(float newValue)
{
	m_light->attenuation.c2 = newValue;
}