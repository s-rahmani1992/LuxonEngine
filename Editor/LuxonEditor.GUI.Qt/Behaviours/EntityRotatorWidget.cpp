#include "EntityRotatorWidget.h"
#include "../Core/QTransformField.h"
#include <QFloatField.h>
#include <QVBoxLayout>
#include <QVector3Field.h>

EntityRotatorWidget::EntityRotatorWidget(QWidget* parent, ref<EntityRotator> rotator)
	: QWidget(parent), m_rotator(rotator)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setAlignment(Qt::AlignTop);

	QTransformField* transformField = new QTransformField(this, "Transform", m_rotator->GetTransform());
	connect(transformField, &QTransformField::ValueChanged, this, [this](ref<LuxonEngine::Transform> transform) {
		m_rotator->SetTransform(transform);
		});
	layout->addWidget(transformField);

	QFloatField* speedField = new QFloatField(this);
	speedField->setLabelText("Rotation Speed");
	speedField->setValue(m_rotator->GetSpeed());
	connect(speedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_rotator->SetSpeed(newSpeed);
		});
	layout->addWidget(speedField);

	QVector3Field* axisField = new QVector3Field(this);
	axisField->setLabelText("Rotation Axis");
	axisField->setValue(m_rotator->GetAxis());
	connect(axisField, &QVector3Field::ValueChanged, this, [this](const LuxonEngine::Vector3& newAxis) {
		m_rotator->SetAxis(newAxis);
		});
	layout->addWidget(axisField);
}

EntityRotatorWidget::~EntityRotatorWidget()
{}

