#include "ShaderInspecterWidget.h"
#include <Core/EngineShaderRegistry.h>
#include <Core/SerializationStream.h>
#include <EngineAPI.h>
#include "ShaderCreationWindow.h"

#define EQUAL_CHAR(a, b) ((a == b) || (a && b && std::strcmp(a, b) == 0))

LuxonEditor::GUI::QT::ShaderInspecterWidget::ShaderInspecterWidget(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent), m_stream(stream)
{
	ui.setupUi(this);
	layout()->setAlignment(ui.propertiesContainer, Qt::AlignTop);
	layout()->setAlignment(ui.buttonPanel, Qt::AlignTop);
	static_cast<QVBoxLayout*>(layout())->addStretch(1);

	ui.propertiesContainer->layout()->setAlignment(ui.shaderTypeField, Qt::AlignTop);
	ui.propertiesContainer->layout()->setAlignment(ui.shaderUsageField, Qt::AlignTop);
	ui.propertiesContainer->layout()->setAlignment(ui.rasterizationContainer, Qt::AlignTop);
	ui.propertiesContainer->layout()->setAlignment(ui.rayTracingContainer, Qt::AlignTop);
	ui.propertiesContainer->layout()->setAlignment(ui.computeContainer, Qt::AlignTop);

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

	ui.vertexField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.vertexField, &QTextField::ValueChanged, this, &ShaderInspecterWidget::OnRasterChanged);

	ui.geometryField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.geometryField, &QNullableTextField::ValueChanged, this, &ShaderInspecterWidget::OnRasterChanged);
	
	ui.pixelField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.pixelField, &QTextField::ValueChanged, this, &ShaderInspecterWidget::OnRasterChanged);

	ui.rayGenField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.rayGenField, &QNullableTextField::ValueChanged, this, &ShaderInspecterWidget::OnRayTracingChanged);
	
	ui.closestHitField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.closestHitField, &QNullableTextField::ValueChanged, this, &ShaderInspecterWidget::OnRayTracingChanged);
	
	ui.missField->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.missField, &QNullableTextField::ValueChanged, this, &ShaderInspecterWidget::OnRayTracingChanged);
	
	ui.computeMain->RegisterValidationFunction(ShaderCreationWindow::FunctionNameValidate);
	connect(ui.computeMain, &QTextField::ValueChanged, this, &ShaderInspecterWidget::OnComputeChanged);

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

		switch (m_newType) {
		case LuxonEngine::Rendering::ShaderProgramType::Rasterization:
			m_currentProperties.rasterProperties.vertexMain = ui.vertexField->GetText();
			m_currentProperties.rasterProperties.geometryMain = ui.geometryField->GetText();
			m_currentProperties.rasterProperties.pixelMain = ui.pixelField->GetText();
			break;
		case LuxonEngine::Rendering::ShaderProgramType::RayTracing:
			m_currentProperties.rayTracingProperties.rayGen = ui.rayGenField->GetText();
			m_currentProperties.rayTracingProperties.closestHit = ui.closestHitField->GetText();
			m_currentProperties.rayTracingProperties.miss = ui.missField->GetText();
			break;
		case LuxonEngine::Rendering::ShaderProgramType::Compute:
			m_currentProperties.computeProperties.computeMain = ui.computeMain->GetText();
			break;
		}

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

	ui.rasterizationContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	ui.rayTracingContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::RayTracing);
	ui.computeContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Compute);

	ui.compileButton->setEnabled(ValidateStageProperties() && ValidateUsageProperties());

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
	ui.compileButton->setEnabled(ValidateStageProperties() && ValidateUsageProperties());
	ui.revertButton->setEnabled(!CompareProperties());
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::SetOriginalValues()
{
	bool isRasterization = m_currentProperties.type == LuxonEngine::Rendering::ShaderProgramType::Rasterization;

	ui.vertexField->InputText()->setText(isRasterization ? m_currentProperties.rasterProperties.vertexMain : "vs_main");

	bool isGeometryActive = isRasterization && m_currentProperties.rasterProperties.geometryMain != nullptr;
	ui.geometryField->Toggle(isGeometryActive);
	ui.geometryField->InputText()->setText(isGeometryActive ? m_currentProperties.rasterProperties.geometryMain : "gs_main");

	ui.pixelField->InputText()->setText(isRasterization ? m_currentProperties.rasterProperties.pixelMain : "ps_main");

	bool isRayTracing = m_currentProperties.type == LuxonEngine::Rendering::ShaderProgramType::RayTracing;
	bool isRayGenActive = isRayTracing && m_currentProperties.rayTracingProperties.rayGen != nullptr;
	ui.rayGenField->Toggle(isRayGenActive);
	ui.rayGenField->InputText()->setText(isRayGenActive ? m_currentProperties.rayTracingProperties.rayGen : "rayGen");

	bool isClosestHitActive = isRayTracing && m_currentProperties.rayTracingProperties.closestHit != nullptr;
	ui.closestHitField->Toggle(isClosestHitActive);
	ui.closestHitField->InputText()->setText(isClosestHitActive ? m_currentProperties.rayTracingProperties.closestHit : "chs");

	bool isMissActive = isRayTracing && m_currentProperties.rayTracingProperties.miss != nullptr;
	ui.missField->Toggle(isMissActive);
	ui.missField->InputText()->setText(isMissActive ? m_currentProperties.rayTracingProperties.miss : "miss");

	bool isCompute = m_currentProperties.type == LuxonEngine::Rendering::ShaderProgramType::Compute;
	ui.computeMain->InputText()->setText(isCompute ? QString::fromUtf8(m_currentProperties.computeProperties.computeMain) : "cs_main");

	ui.shaderTypeBox->setCurrentIndex((int)m_currentProperties.type);

	ui.nameField->InputText()->setText(QString::fromStdString(m_currentProperties.name));
	ui.identifierField->InputText()->setText(QString::fromStdString(m_currentProperties.identifier));
	ui.shaderUsageBox->setCurrentIndex((int)m_currentProperties.usage);
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::ValidateRasterizationProperties()
{
	return ui.vertexField->HasValidValue() && ui.geometryField->HasValidValue() && ui.pixelField->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::ValidateRayTracingProperties()
{
	return ui.rayGenField->HasValidValue() && ui.closestHitField->HasValidValue() && ui.missField->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::ValidateComputeProperties()
{
	return ui.computeMain->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::ValidateUsageProperties()
{
	if (m_newUsage == LuxonEngine::Rendering::ShaderUsage::Internal)
		return ui.identifierField->HasValidValue();

	return ui.nameField->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderInspecterWidget::ValidateStageProperties()
{
	switch (m_newType) {
	case LuxonEngine::Rendering::ShaderProgramType::Rasterization:
		return ValidateRasterizationProperties();
	case LuxonEngine::Rendering::ShaderProgramType::RayTracing:
		return ValidateRayTracingProperties();
	case LuxonEngine::Rendering::ShaderProgramType::Compute:
		return ValidateComputeProperties();
	}
	return false;
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

	if(m_newType == LuxonEngine::Rendering::ShaderProgramType::Rasterization)
	{
		return EQUAL_CHAR(m_currentProperties.rasterProperties.vertexMain, ui.vertexField->GetText()) &&
			EQUAL_CHAR(m_currentProperties.rasterProperties.geometryMain, ui.geometryField->GetText()) &&
			EQUAL_CHAR(m_currentProperties.rasterProperties.pixelMain, ui.pixelField->GetText());
	}

	if(m_newType == LuxonEngine::Rendering::ShaderProgramType::RayTracing)
	{
		return EQUAL_CHAR(m_currentProperties.rayTracingProperties.rayGen, ui.rayGenField->GetText()) &&
			EQUAL_CHAR(m_currentProperties.rayTracingProperties.closestHit, ui.closestHitField->GetText()) &&
			EQUAL_CHAR(m_currentProperties.rayTracingProperties.miss, ui.missField->GetText());
	}

	if(m_newType == LuxonEngine::Rendering::ShaderProgramType::Compute)
	{
		return EQUAL_CHAR(m_currentProperties.computeProperties.computeMain, ui.computeMain->GetText());
	}
	return false;
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::OnRasterChanged(bool text)
{
	if (m_newType != LuxonEngine::Rendering::ShaderProgramType::Rasterization)
		return;

	ui.compileButton->setEnabled(ValidateRasterizationProperties() && ValidateUsageProperties());
	ui.revertButton->setEnabled(!CompareProperties());
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::OnRayTracingChanged(bool isValid)
{
	if (m_newType != LuxonEngine::Rendering::ShaderProgramType::RayTracing)
		return;

	ui.compileButton->setEnabled(ValidateRayTracingProperties() && ValidateUsageProperties());
	ui.revertButton->setEnabled(!CompareProperties());
}

void LuxonEditor::GUI::QT::ShaderInspecterWidget::OnComputeChanged(bool isValid)
{
	if (m_newType != LuxonEngine::Rendering::ShaderProgramType::Compute)
		return;

	ui.compileButton->setEnabled(ValidateComputeProperties() && ValidateUsageProperties());
	ui.revertButton->setEnabled(!CompareProperties());
}
