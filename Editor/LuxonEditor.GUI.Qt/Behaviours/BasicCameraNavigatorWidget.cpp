#include "BasicCameraNavigatorWidget.h"
#include <QBoxLayout>
#include <QFloatField.h>

BasicCameraNavigatorWidget::BasicCameraNavigatorWidget(QWidget* parent, ref<BasicCameraNavigator> cameraNavigator)
	: QWidget(parent), m_cameraNavigator(cameraNavigator)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);

	QLabel* guideLabel = new QLabel("When Holding Right Mouse Button, Use 'WASD' keys and mouse movement to navigate the camera", this);
	guideLabel->setWordWrap(true);
	guideLabel->setStyleSheet(guideLabel->styleSheet() + "QLabel {padding: 5px; border: 1px solid #ffffff; border-radius: 5px; }");
	layout->addWidget(guideLabel);

	QFloatField* forwardSpeedField = new QFloatField(this);
	layout->addWidget(forwardSpeedField);
	forwardSpeedField->setLabelText("Forward Speed");
	forwardSpeedField->setValue(m_cameraNavigator->GetForwardMoveSpeed());

	connect(forwardSpeedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_cameraNavigator->SetForwardMoveSpeed(newSpeed);
		});

	QFloatField* sideSpeedField = new QFloatField(this);
	layout->addWidget(sideSpeedField);
	sideSpeedField->setLabelText("Side Speed");
	sideSpeedField->setValue(m_cameraNavigator->GetSideMoveSpeed());

	connect(sideSpeedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_cameraNavigator->SetSideMoveSpeed(newSpeed);
		});

	QFloatField* rotateSpeedField = new QFloatField(this);
	layout->addWidget(rotateSpeedField);
	rotateSpeedField->setLabelText("Rotate Speed");
	rotateSpeedField->setValue(m_cameraNavigator->GetRotateSpeed());

	connect(rotateSpeedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_cameraNavigator->SetRotateSpeed(newSpeed);
		});
}

BasicCameraNavigatorWidget::~BasicCameraNavigatorWidget()
{}

