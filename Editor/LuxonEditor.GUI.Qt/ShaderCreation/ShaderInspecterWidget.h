#pragma once

#include <QWidget>
#include "ui_ShaderInspecterWidget.h"
#include <LuxonEditorAPI.h>

namespace LuxonEngine {
	class SerializationStream;
}

namespace LuxonEditor::GUI::QT {
	class ShaderInspecterWidget : public QWidget
	{
		Q_OBJECT

	public:
		ShaderInspecterWidget(QWidget* parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
		~ShaderInspecterWidget();

	Q_SIGNALS:
		void PropertyUpdates(LuxonEngine::SerializationStream* stream);

	private:
		void OnShaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType);
		void OnShaderUsageChanged(LuxonEngine::Rendering::ShaderUsage usage);
		void OnUsageFieldChanged(bool isValid);
		void SetOriginalValues();

		bool ValidateRasterizationProperties();
		bool ValidateRayTracingProperties();
		bool ValidateComputeProperties();
		bool ValidateUsageProperties();
		bool ValidateStageProperties();

		bool CompareProperties();

		void OnRasterChanged(bool isValid);
		void OnRayTracingChanged(bool isValid);
		void OnComputeChanged(bool isValid);

		Ui::ShaderInspecterWidgetClass ui;
		LuxonEngine::Rendering::ShaderCompileProperties m_currentProperties;
		LuxonEngine::Rendering::ShaderProgramType m_newType;
		LuxonEngine::Rendering::ShaderUsage m_newUsage;
		LuxonEngine::SerializationStream* m_stream;
	};
}
