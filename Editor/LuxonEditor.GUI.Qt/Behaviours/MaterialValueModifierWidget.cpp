#include "MaterialValueModifierWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include "../Material/QMaterialField.h"
#include <QFloatField.h>
#include <QTextField.h>

MaterialValueModifierWidget::MaterialValueModifierWidget(QWidget* parent, ref<MaterialValueModifier> materialValueModifier)
	: QWidget(parent), m_materialValueModifier(materialValueModifier)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);

	QLabel* guideLabel = new QLabel("use - and + ro increase/decrease value of the material with specified field name between two values", this);
	guideLabel->setWordWrap(true);
	guideLabel->setStyleSheet(guideLabel->styleSheet() + "QLabel {padding: 5px; border: 1px solid #ffffff; border-radius: 5px; }");
	layout->addWidget(guideLabel);

	QMaterialField* materialField = new QMaterialField(this, "material");
	materialField->SetMaterial(materialValueModifier->GetMaterial());
	connect(materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> newMaterial) {
		m_materialValueModifier->SetMaterial(newMaterial);
		});
	layout->addWidget(materialField);

	QTextField* fieldNameField = new QTextField(this, materialValueModifier->GetFieldName());
	fieldNameField->setLabelText("Field Name");
	connect(fieldNameField, &QTextField::ValueChanged, this, [this, fieldNameField](bool isValid) {
		if (isValid) {
			m_materialValueModifier->SetFieldName(fieldNameField->GetStdString());
		}
		});
	layout->addWidget(fieldNameField);

	QFloatField* speedField = new QFloatField(this);
	speedField->setLabelText("Speed");
	speedField->setValue(materialValueModifier->GetSpeed());
	connect(speedField, &QFloatField::ValueChanged, this, [this](float newSpeed) {
		m_materialValueModifier->SetSpeed(newSpeed);
		});
	layout->addWidget(speedField);

	QFloatField* minValueField = new QFloatField(this);
	minValueField->setLabelText("Min Value");
	minValueField->setValue(materialValueModifier->GetMinValue());
	connect(minValueField, &QFloatField::ValueChanged, this, [this](float newMinValue) {
		m_materialValueModifier->SetMinValue(newMinValue);
		});
	layout->addWidget(minValueField);

	QFloatField* maxValueField = new QFloatField(this);
	maxValueField->setLabelText("Max Value");
	maxValueField->setValue(materialValueModifier->GetMaxValue());
	connect(maxValueField, &QFloatField::ValueChanged, this, [this](float newMaxValue) {
		m_materialValueModifier->SetMaxValue(newMaxValue);
		});
	layout->addWidget(maxValueField);

	layout->addStretch();
}
MaterialValueModifierWidget::~MaterialValueModifierWidget()
{}

