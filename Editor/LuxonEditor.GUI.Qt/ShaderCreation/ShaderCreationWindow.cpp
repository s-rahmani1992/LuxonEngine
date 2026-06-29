#include "ShaderCreationWindow.h"
#include <Core/Logger.h>
#include <EngineAPI.h>

LuxonEditor::GUI::QT::ShaderCreationWindow::ShaderCreationWindow(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	OnshaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	connect(ui.shaderTypeBox, &QComboBox::currentIndexChanged, this, [this](int index) {
		LuxonEngine::Rendering::ShaderProgramType type = (LuxonEngine::Rendering::ShaderProgramType)index;

		OnshaderTypeChanged(type);
		});
}

LuxonEditor::GUI::QT::ShaderCreationWindow::~ShaderCreationWindow()
{}

void LuxonEditor::GUI::QT::ShaderCreationWindow::OnshaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType)
{
	ui.rasterizationContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	ui.rayTracingContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::RayTracing);
}

