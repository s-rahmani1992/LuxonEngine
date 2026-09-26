#include "SpikeMeshRendererWidget.h"
#include <QBoxLayout>
#include <LuxonEditorAPI.h>

SpikeMeshRendererWidget::SpikeMeshRendererWidget(QWidget* parent, ref<LuxonEngine::Rendering::SpikeMeshRenderer> spikeMeshRenderer)
	: QWidget(parent), m_spikeMeshRenderer(spikeMeshRenderer)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	setLayout(layout);

	m_meshField = new QMeshField(this, "Mesh");
	m_meshField->SetMesh(spikeMeshRenderer->GetMesh());
	layout->addWidget(m_meshField);
	layout->setAlignment(m_meshField, Qt::AlignTop | Qt::AlignLeft);

	// Material field with Mesh shading filter
	m_materialField = new QMaterialField(this, "Material", LuxonEngine::Rendering::ShaderProgramType::Mesh);
	m_materialField->SetMaterial(spikeMeshRenderer->GetMaterial());
	layout->addWidget(m_materialField);
	layout->setAlignment(m_materialField, Qt::AlignTop | Qt::AlignLeft);

	m_spikeHeightField = new QFloatField(this);
	m_spikeHeightField->setLabelText("Spike Height");
	m_spikeHeightField->setValue(spikeMeshRenderer->GetSpikeHeight());
	layout->addWidget(m_spikeHeightField);
	layout->setAlignment(m_spikeHeightField, Qt::AlignTop | Qt::AlignLeft);

	// --- Connect signals ---

	connect(m_meshField, &QMeshField::ValueChanged, this, [this](ref<LuxonEngine::Mesh> mesh) {
		m_spikeMeshRenderer->SetMesh(mesh);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> material) {
		m_spikeMeshRenderer->SetMaterial(material);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_spikeHeightField, &QFloatField::ValueChanged, this, [this](float value) {
		m_spikeMeshRenderer->SetSpikeHeight(value);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});
}

SpikeMeshRendererWidget::~SpikeMeshRendererWidget()
{
}
