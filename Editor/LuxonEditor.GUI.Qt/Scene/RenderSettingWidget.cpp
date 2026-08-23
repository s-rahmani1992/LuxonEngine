#include "RenderSettingWidget.h"
#include "../Material/QMaterialField.h"
#include "../Texture/QColorField.h"

RenderSettingWidget::RenderSettingWidget(ref<LuxonEngine::Scene> scene, QWidget* parent)
	: QWidget(parent), m_scene(scene)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
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
}

RenderSettingWidget::~RenderSettingWidget()
{}

