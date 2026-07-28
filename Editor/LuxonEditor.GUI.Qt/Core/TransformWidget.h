#pragma once

#include <QWidget>
#include "ui_TransformWidget.h"
#include <EngineAPI.h>
#include <Widgets/QFloatField.h>
#include <Widgets/QVector3Field.h>

class TransformWidget : public QWidget
{
	Q_OBJECT

public:
	TransformWidget(QWidget *parent = nullptr);
	~TransformWidget();
	void SetTransform(const ref<LuxonEngine::Transform>& transform);

Q_SIGNALS:
	void ValueChanged();

private:
	Ui::TransformWidgetClass ui;

	QVector3Field* m_positionField;
	QFloatField* m_rotationAngleField;
	QVector3Field* m_rotationAxisField;
	QVector3Field* m_scaleField;

	ref<LuxonEngine::Transform> m_transform;
};

