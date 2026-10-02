#include "SurfaceInstanceRendererWidget.h"
#include <QBoxLayout>
#include <LuxonEditorAPI.h>

SurfaceInstanceRendererWidget::SurfaceInstanceRendererWidget(QWidget* parent, ref<LuxonEngine::Rendering::SurfaceInstanceRenderer> surfaceInstanceRenderer)
	: QWidget(parent), m_surfaceInstanceRenderer(surfaceInstanceRenderer)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	setLayout(layout);

	m_nearMeshField = new QMeshField(this, "Near Mesh");
	m_nearMeshField->SetMesh(surfaceInstanceRenderer->GetNearMesh());
	layout->addWidget(m_nearMeshField);
	layout->setAlignment(m_nearMeshField, Qt::AlignTop | Qt::AlignLeft);

	m_farMeshField = new QMeshField(this, "Far Mesh");
	m_farMeshField->SetMesh(surfaceInstanceRenderer->GetFarMesh());
	layout->addWidget(m_farMeshField);
	layout->setAlignment(m_farMeshField, Qt::AlignTop | Qt::AlignLeft);

	m_nearDistanceField = new QFloatField(this);
	m_nearDistanceField->setLabelText("Near Distance");
	m_nearDistanceField->setValue(surfaceInstanceRenderer->GetNearDistance());
	layout->addWidget(m_nearDistanceField);
	layout->setAlignment(m_nearDistanceField, Qt::AlignTop | Qt::AlignLeft);

	// Material field with Mesh shading filter
	m_materialField = new QMaterialField(this, "Material", LuxonEngine::Rendering::ShaderProgramType::Mesh);
	m_materialField->SetMaterial(surfaceInstanceRenderer->GetMaterial());
	layout->addWidget(m_materialField);
	layout->setAlignment(m_materialField, Qt::AlignTop | Qt::AlignLeft);

	m_maskTextureField = new QTextureField(this, "Mask Texture");
	m_maskTextureField->SetTexture(surfaceInstanceRenderer->GetMaskTexture());
	layout->addWidget(m_maskTextureField);
	layout->setAlignment(m_maskTextureField, Qt::AlignTop | Qt::AlignLeft);

	m_instanceScaleField = new QFloatField(this);
	m_instanceScaleField->setLabelText("Instance Scale");
	m_instanceScaleField->setValue(surfaceInstanceRenderer->GetInstanceScale());
	layout->addWidget(m_instanceScaleField);
	layout->setAlignment(m_instanceScaleField, Qt::AlignTop | Qt::AlignLeft);

	m_densityField = new QFloatField(this);
	m_densityField->setLabelText("Density");
	m_densityField->setValue(surfaceInstanceRenderer->GetDensity());
	layout->addWidget(m_densityField);
	layout->setAlignment(m_densityField, Qt::AlignTop | Qt::AlignLeft);

	// --- Connect signals ---

	connect(m_nearMeshField, &QMeshField::ValueChanged, this, [this](ref<LuxonEngine::Mesh> mesh) {
		m_surfaceInstanceRenderer->SetNearMesh(mesh);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_farMeshField, &QMeshField::ValueChanged, this, [this](ref<LuxonEngine::Mesh> mesh) {
		m_surfaceInstanceRenderer->SetFarMesh(mesh);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_nearDistanceField, &QFloatField::ValueChanged, this, [this](float value) {
		m_surfaceInstanceRenderer->SetNearDistance(value);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> material) {
		m_surfaceInstanceRenderer->SetMaterial(material);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_maskTextureField, &QTextureField::ValueChanged, this, [this](ref<LuxonEngine::Texture2D> texture) {
		m_surfaceInstanceRenderer->SetMaskTexture(texture);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_instanceScaleField, &QFloatField::ValueChanged, this, [this](float value) {
		m_surfaceInstanceRenderer->SetInstanceScale(value);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_densityField, &QFloatField::ValueChanged, this, [this](float value) {
		m_surfaceInstanceRenderer->SetDensity(value);
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});
}

SurfaceInstanceRendererWidget::~SurfaceInstanceRendererWidget()
{
}
