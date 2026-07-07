#pragma once

#include <QWidget>
#include "ui_ShaderInspecterWidget.h"
#include <LuxonEditorAPI.h>

namespace LuxonEngine {
	class SerializationStream;
}

class ShaderInspecterWidget : public QWidget
{
	Q_OBJECT

public:
	ShaderInspecterWidget(QWidget *parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
	~ShaderInspecterWidget();

private:
	void OnShaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType);

	Ui::ShaderInspecterWidgetClass ui;
	LuxonEngine::Rendering::ShaderCompileProperties m_currentProperties;
	LuxonEngine::Rendering::ShaderCompileProperties m_newProperties;
};

