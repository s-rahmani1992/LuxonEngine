#pragma once

#include <QDialog>
#include "ui_ShaderCreationWindow.h"
#include <EngineAPI.h>

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
		bool ValidateRasterizationProperties();
		bool ValidateRayTracingProperties();
		bool ValidateComputeProperties();

		void OnRasterChanged(bool isValid);
		void OnRayTracingChanged(bool isValid);
		void OnComputeChanged(bool isValid);

		Ui::ShaderCreationWindowClass ui;

		static bool FunctionNameValidate(const QString& text);
		static bool FileNameValidate(const QString& text);

		LuxonEngine::Rendering::ShaderCompileProperties m_compileProperties;
	};
}
