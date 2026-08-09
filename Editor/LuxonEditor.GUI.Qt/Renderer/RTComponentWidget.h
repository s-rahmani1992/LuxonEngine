#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include "../Mesh/QMeshField.h"
#include "../Material/QMaterialField.h"

class RTComponentWidget  : public QWidget
{
	Q_OBJECT

public:
	RTComponentWidget(QWidget *parent);
	~RTComponentWidget();

	void SetRTComponent(ref<LuxonEngine::Rendering::RayTracingComponent> rtComponent);

private:
	QMeshField* m_meshField;
	QMaterialField* m_materialField;
	ref<LuxonEngine::Rendering::RayTracingComponent> m_rtComponent;
};

