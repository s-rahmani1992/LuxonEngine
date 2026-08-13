#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include "../Mesh/QMeshField.h"
#include "../Material/QMaterialField.h"

class GBufferRendererWidget : public QWidget
{
	Q_OBJECT

public:
	GBufferRendererWidget(QWidget* parent);
	void SetGBufferRenderer(ref<LuxonEngine::Rendering::GBufferRTReflectionRenderer> gbufferRenderer);
	~GBufferRendererWidget();

private:
	QMeshField* m_meshField;
	QMaterialField* m_materialField;
	ref<LuxonEngine::Rendering::GBufferRTReflectionRenderer> m_gbufferRenderer;
};