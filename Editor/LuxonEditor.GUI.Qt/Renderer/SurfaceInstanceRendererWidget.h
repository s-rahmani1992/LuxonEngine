#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include "../Mesh/QMeshField.h"
#include "../Material/QMaterialField.h"
#include "../Texture/QTextureField.h"
#include <Widgets/QFloatField.h>

class SurfaceInstanceRendererWidget : public QWidget
{
	Q_OBJECT

public:
	SurfaceInstanceRendererWidget(QWidget* parent, ref<LuxonEngine::Rendering::SurfaceInstanceRenderer> surfaceInstanceRenderer);
	~SurfaceInstanceRendererWidget();

private:
	QMeshField* m_instanceMeshField;
	QMaterialField* m_materialField;
	QTextureField* m_maskTextureField;
	QFloatField* m_instanceScaleField;
	QFloatField* m_densityField;
	ref<LuxonEngine::Rendering::SurfaceInstanceRenderer> m_surfaceInstanceRenderer;
};
