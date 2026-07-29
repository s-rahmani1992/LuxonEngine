#include "ShaderCreationWindow.h"
#include <Core/Logger.h>
#include <LuxonEditorAPI.h>
#include <EngineAPI.h>
#include <filesystem>
#include <StringUtilities.h>

LuxonEditor::GUI::QT::ShaderCreationWindow::ShaderCreationWindow(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	layout()->setAlignment(ui.propertiesContainer, Qt::AlignTop);
	
	OnshaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	connect(ui.shaderTypeBox, &QComboBox::currentIndexChanged, this, [this](int index) {
		LuxonEngine::Rendering::ShaderProgramType type = (LuxonEngine::Rendering::ShaderProgramType)index;

		OnshaderTypeChanged(type);
		});

	int i = 0;
	while (FileNameValidate("new_shader_" + QString::number(i)) == false)
	{
		i++;
	};

	ui.shaderNameField->InputText()->setText("new_shader_" + QString::number(i));
	ui.shaderNameField->RegisterValidationFunction(FileNameValidate);
	connect(ui.shaderNameField, &QTextField::ValueChanged, this, [this](bool isValid) {
		ui.createButton->setEnabled(isValid);
		});

	ui.vertexField->InputText()->setText("vs_main");
	ui.vertexField->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.vertexField, &QTextField::ValueChanged, this, &ShaderCreationWindow::OnRasterChanged);

	ui.geometryField->InputText()->setText("gs_main");
	ui.geometryField->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.geometryField, &QNullableTextField::ValueChanged, this, &ShaderCreationWindow::OnRasterChanged);

	ui.pixelField->InputText()->setText("ps_main");
	ui.pixelField->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.pixelField, &QTextField::ValueChanged, this, &ShaderCreationWindow::OnRasterChanged);

	ui.rayGenField->InputText()->setText("rayGen");
	ui.rayGenField->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.rayGenField, &QNullableTextField::ValueChanged, this, &ShaderCreationWindow::OnRayTracingChanged);

	ui.closestHitField->InputText()->setText("chs");
	ui.closestHitField->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.closestHitField, &QNullableTextField::ValueChanged, this, &ShaderCreationWindow::OnRayTracingChanged);

	ui.missField->InputText()->setText("miss");
	ui.missField->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.missField, &QNullableTextField::ValueChanged, this, &ShaderCreationWindow::OnRayTracingChanged);

	ui.computeMain->InputText()->setText("cs_main");
	ui.computeMain->RegisterValidationFunction(FunctionNameValidate);
	connect(ui.computeMain, &QTextField::ValueChanged, this, &ShaderCreationWindow::OnComputeChanged);

	connect(ui.createButton, &QPushButton::clicked, this, [this]() {
		m_compileProperties.model = "6_6";
		switch (m_compileProperties.type) {
		case LuxonEngine::Rendering::ShaderProgramType::Rasterization:
			vertexMainStr = ui.vertexField->InputText()->text().toStdString();
			m_compileProperties.rasterProperties.vertexMain = vertexMainStr.data();
			m_compileProperties.rasterProperties.geometryMain = ui.geometryField->GetText();
			pixelMainStr = ui.pixelField->InputText()->text().toStdString();
			m_compileProperties.rasterProperties.pixelMain = pixelMainStr.data();
			break;
		case LuxonEngine::Rendering::ShaderProgramType::RayTracing:
			m_compileProperties.rayTracingProperties.rayGen = ui.rayGenField->GetText();
			m_compileProperties.rayTracingProperties.closestHit = ui.closestHitField->GetText();
			m_compileProperties.rayTracingProperties.miss = ui.missField->GetText();
			break;
		case LuxonEngine::Rendering::ShaderProgramType::Compute:
			computeMainStr = ui.computeMain->InputText()->text().toStdString();
			m_compileProperties.computeProperties.computeMain = computeMainStr.data();
			break;
		}
		CreateShader(m_compileProperties, ui.shaderNameField->InputText()->text().toStdString());
		accept();
		});

	connect(ui.cancelButton, &QPushButton::clicked, this, [this]() {
		reject();
		});
}

LuxonEditor::GUI::QT::ShaderCreationWindow::~ShaderCreationWindow()
{}

void LuxonEditor::GUI::QT::ShaderCreationWindow::OnshaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType)
{
	m_compileProperties.type = programType;
	ui.rasterizationContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	ui.rayTracingContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::RayTracing);
	ui.computeContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Compute);

	switch (programType) {
		case LuxonEngine::Rendering::ShaderProgramType::Rasterization:
			ui.createButton->setEnabled(ValidateRasterizationProperties());
			break;
		case LuxonEngine::Rendering::ShaderProgramType::RayTracing:
			ui.createButton->setEnabled(ValidateRayTracingProperties());
			break;
		case LuxonEngine::Rendering::ShaderProgramType::Compute:
			ui.createButton->setEnabled(ValidateComputeProperties());
			break;
	}
}

bool LuxonEditor::GUI::QT::ShaderCreationWindow::ValidateRasterizationProperties()
{
	return ui.vertexField->HasValidValue() && ui.geometryField->HasValidValue() && ui.pixelField->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderCreationWindow::ValidateRayTracingProperties()
{
	return ui.rayGenField->HasValidValue() && ui.closestHitField->HasValidValue() && ui.missField->HasValidValue();
}

bool LuxonEditor::GUI::QT::ShaderCreationWindow::ValidateComputeProperties()
{
	return ui.computeMain->HasValidValue();
}

void LuxonEditor::GUI::QT::ShaderCreationWindow::OnRasterChanged(bool text)
{
	if(m_compileProperties.type != LuxonEngine::Rendering::ShaderProgramType::Rasterization)
		return;

	ui.createButton->setEnabled(ValidateRasterizationProperties() && FileNameValidate(ui.shaderNameField->InputText()->text()));
}

void LuxonEditor::GUI::QT::ShaderCreationWindow::OnRayTracingChanged(bool isValid)
{
	if (m_compileProperties.type != LuxonEngine::Rendering::ShaderProgramType::RayTracing)
		return;
	ui.createButton->setEnabled(ValidateRayTracingProperties() && FileNameValidate(ui.shaderNameField->InputText()->text()));
}

void LuxonEditor::GUI::QT::ShaderCreationWindow::OnComputeChanged(bool isValid)
{
	if (m_compileProperties.type != LuxonEngine::Rendering::ShaderProgramType::Compute)
		return;
	ui.createButton->setEnabled(ValidateComputeProperties() && FileNameValidate(ui.shaderNameField->InputText()->text()));
}

bool LuxonEditor::GUI::QT::ShaderCreationWindow::FunctionNameValidate(const QString& text)
{
	// Empty check
	if (text.isEmpty())
	{
		return false;
	}

	// Check if the first character is a letter or underscore
	QChar firstChar = text.at(0);
	if (!firstChar.isLetter() && firstChar != '_')
	{
		return false;
	}

	// Check if all characters are letters, digits, or underscores
	for (int i = 0; i < text.length(); ++i)
	{
		QChar currentChar = text.at(i);
		if (!currentChar.isLetterOrNumber() && currentChar != '_')
		{
			return false;
		}
	}

	return true;
}

bool LuxonEditor::GUI::QT::ShaderCreationWindow::FileNameValidate(const QString& text)
{
	if(FunctionNameValidate(text) == false)
	{
		return false;
	}

	auto shaderFolderPath = GetProjectPath() + "/Assets/Shaders/";
	// implement checking if the file exists in the shader folder
	return !std::filesystem::exists(shaderFolderPath + text.toStdString() + ".hlsl");
}

