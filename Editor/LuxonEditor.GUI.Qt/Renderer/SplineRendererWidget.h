#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include "../Material/QMaterialField.h"
#include <Widgets/QVector3Field.h>
#include <Widgets/QFloatField.h>

class QSpinBox;

class SplineRendererWidget : public QWidget
{
	Q_OBJECT

public:
	SplineRendererWidget(QWidget* parent, ref<LuxonEngine::Rendering::SplineRenderer> splineRenderer);
	~SplineRendererWidget();

private:
	QVector3Field* m_point1Field;
	QVector3Field* m_point2Field;
	QVector3Field* m_point3Field;

	QFloatField* m_widthField;
	QSpinBox* m_segmentsField;

	QMaterialField* m_materialField;

	ref<LuxonEngine::Rendering::SplineRenderer> m_splineRenderer;
};