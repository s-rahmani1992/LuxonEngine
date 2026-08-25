#pragma once

#include <QWidget>
#include "ui_QTransformField.h"
#include <EngineAPI.h>

class QTransformField : public QWidget
{
	Q_OBJECT

		Q_PROPERTY(QString label READ GetLabel WRITE SetLabel)

public:
	QTransformField(QWidget* parent = nullptr, std::string fieldName = "", ref<LuxonEngine::Transform> transform = nullptr);
	~QTransformField();

	void SetTransform(ref<LuxonEngine::Transform> transform);

	QString GetLabel() const { return ui.label->text(); }
	void SetLabel(const QString& label) { ui.label->setText(label); }

Q_SIGNALS:
	void ValueChanged(ref<LuxonEngine::Transform> newTransform);

private:
	Ui::QTransformFieldClass ui;

	ref<LuxonEngine::Transform> m_transform;
};

