#pragma once

#include <QWidget>
#include "ui_QMaterialField.h"
#include <EngineAPI.h>
#include <Rendering/ShaderProgram.h>

class QMaterialField : public QWidget
{
	Q_OBJECT

		Q_PROPERTY(QString label READ GetLabel WRITE SetLabel)

public:
	QMaterialField(QWidget* parent = nullptr, std::string fieldName = "",
		LuxonEngine::Rendering::ShaderProgramType programType = LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	~QMaterialField();

	void SetMaterial(ref<LuxonEngine::Rendering::Material> material);

	QString GetLabel() const { return ui.label->text(); }
	void SetLabel(const QString& label) { ui.label->setText(label); }

Q_SIGNALS:
	void ValueChanged(ref<LuxonEngine::Rendering::Material> newMaterial);

private:
	Ui::QMaterialFieldClass ui;
	ref<LuxonEngine::Rendering::Material> m_material;
	LuxonEngine::Rendering::ShaderProgramType m_programType;
};