#include "PerspectiveCameraWidget.h"
#include <QVBoxLayout>

PerspectiveCameraWidget::PerspectiveCameraWidget(QWidget* parent)
	: QWidget(parent)
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setAlignment(Qt::AlignTop);
	setLayout(mainLayout);

	m_transformWidget = new TransformWidget(this);
	m_transformWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(m_transformWidget);

	m_nearZField = new QFloatField(this);
	m_nearZField->setLabelText("Near Z");
	m_nearZField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(m_nearZField);

	m_farZField = new QFloatField(this);
	m_farZField->setLabelText("Far Z");
	m_farZField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(m_farZField);

	m_fovAngleField = new QFloatField(this);
	m_fovAngleField->setLabelText("FOV Angle");
	m_fovAngleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(m_fovAngleField);

	connect(m_transformWidget, &TransformWidget::ValueChanged, this, [this]() {
		emit ValueChanged();
		});

	connect(m_nearZField, &QFloatField::ValueChanged, this, [this](float value) {
		if (m_camera) {
			m_camera->SetNearZ(value);
			emit ValueChanged();
		}
		});

	connect(m_farZField, &QFloatField::ValueChanged, this, [this](float value) {
		if (m_camera) {
			m_camera->SetFarZ(value);
			emit ValueChanged();
		}
		});

	connect(m_fovAngleField, &QFloatField::ValueChanged, this, [this](float value) {
		if (m_camera) {
			m_camera->SetFovAngle(value);
			emit ValueChanged();
		}
		});
}

PerspectiveCameraWidget::~PerspectiveCameraWidget()
{
}

void PerspectiveCameraWidget::SetCamera(const ref<LuxonEngine::PerspectiveCamera>& camera)
{
	m_camera = camera;
	if (m_camera) {
		m_transformWidget->SetTransform(m_camera->GetTransform());
		m_nearZField->setValue(m_camera->GetNearZ());
		m_farZField->setValue(m_camera->GetFarZ());
		m_fovAngleField->setValue(m_camera->GetFovAngle());
	}
}