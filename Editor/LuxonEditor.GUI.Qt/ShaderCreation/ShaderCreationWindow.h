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
		static bool FunctionNameValidate(const QString& text);
		static bool NameValidate(const QString& text);
	private:
		void OnshaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType);
		void OnShaderUsageChanged(LuxonEngine::Rendering::ShaderUsage usage);
		void UpdateCreateButton();
		bool ValidateRasterizationProperties();
		bool ValidateRayTracingProperties();
		bool ValidateComputeProperties();
		bool ValidateMeshProperties();
		bool ValidateUsageProperties();

		void OnRasterChanged(bool isValid);
		void OnRayTracingChanged(bool isValid);
		void OnComputeChanged(bool isValid);
		void OnMeshChanged(bool isValid);

		std::string computeMainStr;
		std::string vertexMainStr;
		std::string pixelMainStr;
		std::string meshMainStr;

		Ui::ShaderCreationWindowClass ui;

		
		static bool FileNameValidate(const QString& text);

		LuxonEngine::Rendering::ShaderCompileProperties m_compileProperties;
	};
}
