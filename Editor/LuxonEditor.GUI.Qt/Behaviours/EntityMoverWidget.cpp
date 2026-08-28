#include "EntityMoverWidget.h"
#include <qboxlayout.h>
#include "../Core/QTransformField.h"
#include <QFloatField.h>
#include <QVector3Field.h>

EntityMoverWidget::EntityMoverWidget(QWidget* parent, ref<EntityMover> entityMover)
	: QWidget(parent), m_entityMover(entityMover)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setAlignment(Qt::AlignTop);

	QLabel* guideLabel = new QLabel("Moves the entity between two points", this);
	guideLabel->setWordWrap(true);
	guideLabel->setStyleSheet(guideLabel->styleSheet() + "QLabel {padding: 5px; border: 1px solid #ffffff; border-radius: 5px; }");
	layout->addWidget(guideLabel);

	QTransformField* transformField = new QTransformField(this, "Transform", entityMover->GetTransform());
	connect(transformField, &QTransformField::ValueChanged, this, [this](ref<LuxonEngine::Transform> transform) {
		m_entityMover->SetTransform(transform);
		});
	layout->addWidget(transformField);

	QVector3Field* point1Field = new QVector3Field(this);
	point1Field->setLabelText("Point 1");
	point1Field->setValue(m_entityMover->GetPoint1());
	connect(point1Field, &QVector3Field::ValueChanged, this, [this](const LuxonEngine::Vector3& newPoint1) {
		m_entityMover->SetPoint1(newPoint1);
		});
	layout->addWidget(point1Field);

	QVector3Field* point2Field = new QVector3Field(this);
	point2Field->setLabelText("Point 2");
	point2Field->setValue(m_entityMover->GetPoint2());
	connect(point2Field, &QVector3Field::ValueChanged, this, [this](const LuxonEngine::Vector3& newPoint2) {
		m_entityMover->SetPoint2(newPoint2);
		});
	layout->addWidget(point2Field);

	QFloatField* startField = new QFloatField(this);
	startField->setLabelText("Start Position (0-1)");
	startField->setValue(m_entityMover->GetStart());
	connect(startField, &QFloatField::ValueChanged, this, [this](float newStart) {
		m_entityMover->SetStart(newStart);
		});
	layout->addWidget(startField);

	QFloatField* speedField = new QFloatField(this);
	speedField->setLabelText("Movement Speed");
	speedField->setValue(m_entityMover->GetSpeed());
	connect(speedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_entityMover->SetSpeed(newSpeed);
		});
	layout->addWidget(speedField);
}

EntityMoverWidget::~EntityMoverWidget()
{}

