#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include "../Mesh/QMeshField.h"
#include "../Material/QMaterialField.h"
#include <Widgets/QFloatField.h>

class SpikeMeshRendererWidget : public QWidget
{
	Q_OBJECT

public:
	SpikeMeshRendererWidget(QWidget* parent, ref<LuxonEngine::Rendering::SpikeMeshRenderer> spikeMeshRenderer);
	~SpikeMeshRendererWidget();

private:
	QMeshField* m_meshField;
	QMaterialField* m_materialField;
	QFloatField* m_spikeHeightField;
	ref<LuxonEngine::Rendering::SpikeMeshRenderer> m_spikeMeshRenderer;
};
