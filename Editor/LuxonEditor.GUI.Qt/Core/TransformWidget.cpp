#include "TransformWidget.h"

TransformWidget::TransformWidget(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	m_positionField = new QVector3Field(this);
	layout()->addWidget(m_positionField);
	layout()->setAlignment(m_positionField, Qt::AlignTop);
	m_positionField->setLabelText("Position");
	m_positionField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	m_rotationAngleField = new QFloatField(this);
	layout()->addWidget(m_rotationAngleField);
	layout()->setAlignment(m_rotationAngleField, Qt::AlignTop);
	m_rotationAngleField->setLabelText("Rotation Angle");
	m_rotationAngleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	m_rotationAxisField = new QVector3Field(this);
	layout()->addWidget(m_rotationAxisField);
	layout()->setAlignment(m_rotationAxisField, Qt::AlignTop);
	m_rotationAxisField->setLabelText("Rotation Axis");
	m_rotationAxisField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	m_scaleField = new QVector3Field(this);
	layout()->addWidget(m_scaleField);
	layout()->setAlignment(m_scaleField, Qt::AlignTop);
	m_scaleField->setLabelText("Scale");
	m_scaleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(m_positionField, &QVector3Field::ValueChanged, this, [this](const LuxonEngine::Vector3& value) {
		if (m_transform) {
			m_transform->SetPosition(value);
			emit ValueChanged();
		}
		});

	connect(m_rotationAxisField, &QVector3Field::ValueChanged, this, [this](const LuxonEngine::Vector3& value) {
		if (m_transform) {
			m_transform->SetRotation(value, m_rotationAngleField->value());
			emit ValueChanged();
		}
		});

	connect(m_rotationAngleField, &QFloatField::ValueChanged, this, [this](float value) {
		if (m_transform) {
			m_transform->SetRotation(m_rotationAxisField->value(), value);
			emit ValueChanged();
		}
		});

	connect(m_scaleField, &QVector3Field::ValueChanged, this, [this](const LuxonEngine::Vector3& value) {
		if (m_transform) {
			m_transform->SetScale(value);
			emit ValueChanged();
		}
		});
}

TransformWidget::~TransformWidget()
{}

void TransformWidget::SetTransform(const ref<LuxonEngine::Transform>&transform)
{
	m_transform = transform;

	m_positionField->setValue(m_transform->Position());

	auto n = m_transform->RotationAxis();
	m_rotationAxisField->setValue(m_transform->RotationAxis());
	m_rotationAngleField->setValue(m_transform->GetAngle());
	m_scaleField->setValue(m_transform->Scale());
}

