#include "EntityPositionControllerWidget.h"
#include <QVBoxLayout>
#include "../Core/QTransformField.h"
#include <QFloatField.h>

EntityPositionControllerWidget::EntityPositionControllerWidget(QWidget* parent, ref<EntityPositionController> entityPositionController)
	:QWidget(parent), m_entityPositionController(entityPositionController)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);

	QLabel* guideLabel = new QLabel("Use arrow keys to move the entity horizontally in the xz plane", this);
	guideLabel->setWordWrap(true);
	guideLabel->setStyleSheet(guideLabel->styleSheet() + "QLabel {padding: 5px; border: 1px solid #ffffff; border-radius: 5px; }");
	layout->addWidget(guideLabel);

	QTransformField* transformField = new QTransformField(this, "Transform", m_entityPositionController->GetTransform());
	connect(transformField, &QTransformField::ValueChanged, this, [this](ref<LuxonEngine::Transform> transform) {
		m_entityPositionController->SetTransform(transform);
		});
	layout->addWidget(transformField);

	QFloatField* speedField = new QFloatField(this);
	speedField->setLabelText("Movement Speed");
	speedField->setValue(m_entityPositionController->GetSpeed());
	connect(speedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_entityPositionController->SetSpeed(newSpeed);
		});
	layout->addWidget(speedField);

	layout->addStretch();
}

EntityPositionControllerWidget::~EntityPositionControllerWidget()
{}

