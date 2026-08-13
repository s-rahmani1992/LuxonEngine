#include "GBufferRendererWidget.h"
#include <QBoxLayout>
#include <LuxonEditorAPI.h>

GBufferRendererWidget::GBufferRendererWidget(QWidget* parent)
	: QWidget(parent)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	setLayout(layout);

	m_meshField = new QMeshField(this, "Mesh");
	layout->addWidget(m_meshField);
	layout->setAlignment(m_meshField, Qt::AlignTop | Qt::AlignLeft);

	m_materialField = new QMaterialField(this, "Material", LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	layout->addWidget(m_materialField);
	layout->setAlignment(m_materialField, Qt::AlignTop | Qt::AlignLeft);

	connect(m_meshField, &QMeshField::ValueChanged, this, [this](ref<LuxonEngine::Mesh> mesh) {
		if (m_gbufferRenderer) {
			m_gbufferRenderer->SetMesh(mesh);
			LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		}
		});

	connect(m_materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> material) {
		if (m_gbufferRenderer) {
			m_gbufferRenderer->SetMaterial(material);
		}
		});
}

void GBufferRendererWidget::SetGBufferRenderer(ref<LuxonEngine::Rendering::GBufferRTReflectionRenderer> gbufferRenderer)
{
	m_gbufferRenderer = gbufferRenderer;
	m_meshField->SetMesh(gbufferRenderer->GetMesh());
	m_materialField->SetMaterial(gbufferRenderer->GetMaterial());
}

GBufferRendererWidget::~GBufferRendererWidget()
{
}