#pragma once

#include <QWidget>
#include "ui_QMeshField.h"
#include <EngineAPI.h>

class QMeshField : public QWidget
{
	Q_OBJECT

		Q_PROPERTY(QString label READ GetLabel WRITE SetLabel)

public:
	QMeshField(QWidget* parent = nullptr, std::string fieldName = "");
	~QMeshField();

	void SetMesh(ref<LuxonEngine::Mesh> mesh);

	QString GetLabel() const { return ui.label->text(); }
	void SetLabel(const QString& label) { ui.label->setText(label); }

Q_SIGNALS:
	void ValueChanged(ref<LuxonEngine::Mesh> newMesh);

private:
	Ui::QMeshFieldClass ui;
	ref<LuxonEngine::Mesh> m_mesh;
};