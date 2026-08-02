#include "MeshRendererWidget.h"
#include <QBoxLayout>

MeshRendererWidget::MeshRendererWidget(QWidget *parent)
	: QWidget(parent)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	setLayout(layout);

	m_meshField = new QMeshField(this, "Mesh");
	layout->addWidget(m_meshField);
	layout->setAlignment(m_meshField, Qt::AlignTop | Qt::AlignLeft);

	m_materialField = new QMaterialField(this, "Material");
	layout->addWidget(m_materialField);
	layout->setAlignment(m_materialField, Qt::AlignTop | Qt::AlignLeft);
	connect(m_meshField, &QMeshField::ValueChanged, this, [this](ref<LuxonEngine::Mesh> mesh) {
		if (m_meshRenderer) {
			m_meshRenderer->SetMesh(mesh);
		}
		});

	connect(m_materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> material) {
		if (m_meshRenderer) {
			m_meshRenderer->SetMaterial(material);
		}
		});
}

void MeshRendererWidget::SetMeshRenderer(ref<LuxonEngine::Rendering::MeshRenderer> meshRenderer)
{
	m_meshRenderer = meshRenderer;
	m_meshField->SetMesh(meshRenderer->GetMesh());
	m_materialField->SetMaterial(meshRenderer->GetMaterial());
}

MeshRendererWidget::~MeshRendererWidget()
{}

