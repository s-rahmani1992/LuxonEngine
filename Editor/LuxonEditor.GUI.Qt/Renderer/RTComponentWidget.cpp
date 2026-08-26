#include "RTComponentWidget.h"
#include <QBoxLayout>
#include <LuxonEditorAPI.h>

RTComponentWidget::RTComponentWidget(QWidget *parent)
	: QWidget(parent)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	setLayout(layout);

	m_meshField = new QMeshField(this, "Mesh");
	layout->addWidget(m_meshField);
	layout->setAlignment(m_meshField, Qt::AlignTop | Qt::AlignLeft);

	m_materialField = new QMaterialField(this, "Material", LuxonEngine::Rendering::ShaderProgramType::RayTracing);
	layout->addWidget(m_materialField);
	layout->setAlignment(m_materialField, Qt::AlignTop | Qt::AlignLeft);
	connect(m_meshField, &QMeshField::ValueChanged, this, [this](ref<LuxonEngine::Mesh> mesh) {
		if (m_rtComponent) {
			m_rtComponent->SetMesh(mesh);
			LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		}
		});

	connect(m_materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> material) {
		if (m_rtComponent) {
			m_rtComponent->SetRTMaterial(material);
		}
		});
}

RTComponentWidget::~RTComponentWidget()
{}

void RTComponentWidget::SetRTComponent(ref<LuxonEngine::Rendering::RayTracingComponent> rtComponent)
{
	m_rtComponent = rtComponent;
	m_meshField->SetMesh(rtComponent->GetMesh());
	m_materialField->SetMaterial(rtComponent->GetRTMaterial());
}

