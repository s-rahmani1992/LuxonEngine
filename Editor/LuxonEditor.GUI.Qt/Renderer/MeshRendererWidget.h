#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include "../Mesh/QMeshField.h"
#include "../Material/QMaterialField.h"

class MeshRendererWidget  : public QWidget
{
	Q_OBJECT

public:
	MeshRendererWidget(QWidget *parent);
	void SetMeshRenderer(ref<LuxonEngine::Rendering::MeshRenderer> meshRenderer);
	~MeshRendererWidget();

private:
	QMeshField* m_meshField;
	QMaterialField* m_materialField;
	ref<LuxonEngine::Rendering::MeshRenderer> m_meshRenderer;
};

