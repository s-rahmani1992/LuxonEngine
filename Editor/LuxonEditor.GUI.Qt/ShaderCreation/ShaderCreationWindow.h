#pragma once

#include <QDialog>
#include "ui_ShaderCreationWindow.h"

namespace LuxonEngine::Rendering {
	enum class ShaderProgramType;
}

namespace LuxonEditor::GUI::QT {
	class ShaderCreationWindow : public QDialog
	{
		Q_OBJECT

	public:
		ShaderCreationWindow(QWidget* parent = nullptr);
		~ShaderCreationWindow();

	private:
		void OnshaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType);
		Ui::ShaderCreationWindowClass ui;
	};
}
