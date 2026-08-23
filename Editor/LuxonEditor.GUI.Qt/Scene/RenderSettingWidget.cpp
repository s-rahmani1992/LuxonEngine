#include "RenderSettingWidget.h"
#include "../Material/QMaterialField.h"
#include "../Texture/QColorField.h"
#include <qcheckbox.h>
#include <LuxonEditorAPI.h>

RenderSettingWidget::RenderSettingWidget(ref<LuxonEngine::Scene> scene, QWidget* parent)
	: QWidget(parent), m_scene(scene)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	setLayout(layout);
	QMaterialField* materialField = new QMaterialField(this, "Global RT Material", LuxonEngine::Rendering::ShaderProgramType::RayTracing);
	layout->addWidget(materialField);
	materialField->SetMaterial(m_scene->rtGlobalMaterial);
	connect(materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> newMaterial) {
		m_scene->rtGlobalMaterial = newMaterial;
		});

	QColorField* colorField = new QColorField(this, "Hybrid Background Color");
	layout->addWidget(colorField);
	colorField->SetColor(m_scene->hybridBackgroundColor);
	connect(colorField, &QColorField::ValueChanged, this, [this](const LuxonEngine::Color& newColor) {
		m_scene->hybridBackgroundColor = newColor;
		});

	QCheckBox* rayTracingSupportCheckBox = new QCheckBox("Enable Ray Tracing Support", this);
	rayTracingSupportCheckBox->setChecked(m_scene->canSupportRayTracing);
	layout->addWidget(rayTracingSupportCheckBox);
	connect(rayTracingSupportCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
		LuxonEditor::EngineApplication::GetSceneManager()->SetRayTracingSupportEnabled(checked);
		});

	QCheckBox* hybridRenderingSupportCheckBox = new QCheckBox("Enable Hybrid Rendering Support", this);
	hybridRenderingSupportCheckBox->setChecked(m_scene->canSupportHybridRendering);
	layout->addWidget(hybridRenderingSupportCheckBox);
	connect(hybridRenderingSupportCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
		LuxonEditor::EngineApplication::GetSceneManager()->SetHybridSupportEnabled(checked);
		});
}

RenderSettingWidget::~RenderSettingWidget()
{}

