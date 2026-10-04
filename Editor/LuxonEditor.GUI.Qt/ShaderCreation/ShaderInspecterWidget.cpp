#include "ShaderInspecterWidget.h"
#include <Core/EngineShaderRegistry.h>
#include <Core/SerializationStream.h>
#include <EngineAPI.h>
#include "ShaderCreationWindow.h"

LuxonEditor::GUI::QT::ShaderInspecterWidget::ShaderInspecterWidget(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent), m_stream(stream)
{
	ui.setupUi(this);
	layout()->setAlignment(ui.propertiesContainer, Qt::AlignTop);
	layout()->setAlignment(ui.buttonPanel, Qt::AlignTop);
	static_cast<QVBoxLayout*>(layout())->addStretch(1);

	ui.propertiesContainer->layout()->setAlignment(ui.shaderTypeField, Qt::AlignTop);
	ui.propertiesContainer->layout()->setAlignment(ui.shaderUsageField, Qt::AlignTop);

	ui.shaderTypeField->layout()->setAlignment(ui.label, Qt::AlignLeft);
	ui.shaderUsageField->layout()->setAlignment(ui.usageLabel, Qt::AlignLeft);

	if (stream == nullptr) {
		return;
	}
	LuxonEditor::EngineShaderRegistry::FillProperties(m_currentProperties, *stream);
	m_newType = m_currentProperties.type;
	m_newUsage = m_currentProperties.usage;

	SetOriginalValues();

	ui.nameField->RegisterValidationFunction(ShaderCreationWindow::NameValidate);
	connect(ui.nameField, &QTextField::ValueChanged, this, &ShaderInspecterWidget::OnUsageFieldChanged);

	ui.identifierField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.identifierField, &QTextField::ValueChanged, this, &ShaderInspecterWidget::OnUsageFieldChanged);

	OnShaderTypeChanged(m_newType);
	OnShaderUsageChanged(m_newUsage);

	connect(ui.shaderTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
		LuxonEngine::Rendering::ShaderProgramType type = (LuxonEngine::Rendering::ShaderProgramType)index;
		OnShaderTypeChanged(type);
		});

	connect(ui.shaderUsageBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
		OnShaderUsageChanged((LuxonEngine::Rendering::ShaderUsage)index);
		});

	connect(ui.revertButton, &QPushButton::clicked, this, [this]() {
		SetOriginalValues();
		});

	connect(ui.compileButton, &QPushButton::clicked, this, [this]() {
		m_currentProperties.type = m_newType;
		m_currentProperties.usage = m_newUsage;

		bool isInternal = m_newUsage == LuxonEngine::Rendering::ShaderUsage::Internal;
		m_currentProperties.identifier = isInternal ? ui.identifierField->GetStdString() : std::string();
		m_currentProperties.name = isInternal ? std::string() : ui.nameField->GetStdString();

		EngineShaderRegistry::SerializeProperties(m_currentProperties, *m_stream);
		EngineShaderRegistry::FillProperties(m_currentProperties, *m_stream);
		emit PropertyUpdates(m_stream);
		ui.revertButton->setEnabled(false);
		});
}

LuxonEditor::GUI::QT::ShaderInspecterWidget::~ShaderInspecterWidget()
{
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::OnShaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType)
{
	m_newType = programType;

	ui.compileButton->setEnabled(ValidateUsageProperties());

	bool equal = CompareProperties();

	ui.revertButton->setEnabled(!equal);
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::OnShaderUsageChanged(LuxonEngine::Rendering::ShaderUsage usage)
{
	m_newUsage = usage;

	ui.nameField->setVisible(usage == LuxonEngine::Rendering::ShaderUsage::User);
	ui.identifierField->setVisible(usage == LuxonEngine::Rendering::ShaderUsage::Internal);

	OnUsageFieldChanged(true);
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::OnUsageFieldChanged(bool isValid)
{
	ui.compileButton->setEnabled(ValidateUsageProperties());
	ui.revertButton->setEnabled(!CompareProperties());
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::SetOriginalValues()
{
	ui.shaderTypeBox->setCurrentIndex((int)m_currentProperties.type);

	ui.nameField->InputText()->setText(QString::fromStdString(m_currentProperties.name));
	ui.identifierField->InputText()->setText(QString::fromStdString(m_currentProperties.identifier));
	ui.shaderUsageBox->setCurrentIndex((int)m_currentProperties.usage);
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::ValidateUsageProperties()
{
	if (m_newUsage == LuxonEngine::Rendering::ShaderUsage::Internal)
		return ui.identifierField->HasValidValue();

	return ui.nameField->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::CompareProperties()
{
	if(m_newType != m_currentProperties.type)
		return false;

	if (m_newUsage != m_currentProperties.usage)
		return false;

	if (m_newUsage == LuxonEngine::Rendering::ShaderUsage::Internal && ui.identifierField->GetStdString() != m_currentProperties.identifier)
		return false;

	if (m_newUsage == LuxonEngine::Rendering::ShaderUsage::User && ui.nameField->GetStdString() != m_currentProperties.name)
		return false;

	return true;
}
