#include "SplineRendererWidget.h"
#include <QBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <LuxonEditorAPI.h>

SplineRendererWidget::SplineRendererWidget(QWidget* parent, ref<LuxonEngine::Rendering::SplineRenderer> splineRenderer)
	: QWidget(parent), m_splineRenderer(splineRenderer)
{
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	setLayout(layout);

	const auto& curve = splineRenderer->GetCurve();

	// Point fields
	m_point1Field = new QVector3Field(this);
	m_point1Field->setLabelText("Point 1");
	m_point1Field->setValue(curve.m_point1);
	layout->addWidget(m_point1Field);
	layout->setAlignment(m_point1Field, Qt::AlignTop | Qt::AlignLeft);

	m_point2Field = new QVector3Field(this);
	m_point2Field->setLabelText("Point 2");
	m_point2Field->setValue(curve.m_point2);
	layout->addWidget(m_point2Field);
	layout->setAlignment(m_point2Field, Qt::AlignTop | Qt::AlignLeft);

	m_point3Field = new QVector3Field(this);
	m_point3Field->setLabelText("Point 3");
	m_point3Field->setValue(curve.m_point3);
	layout->addWidget(m_point3Field);
	layout->setAlignment(m_point3Field, Qt::AlignTop | Qt::AlignLeft);

	// Width field
	m_widthField = new QFloatField(this);
	m_widthField->setLabelText("Width");
	m_widthField->setValue(splineRenderer->GetWidth());
	layout->addWidget(m_widthField);
	layout->setAlignment(m_widthField, Qt::AlignTop | Qt::AlignLeft);

	// Segments field
	QWidget* segmentsRow = new QWidget(this);
	QBoxLayout* segmentsLayout = new QBoxLayout(QBoxLayout::LeftToRight, segmentsRow);
	segmentsLayout->setContentsMargins(0, 0, 0, 0);
	segmentsRow->setLayout(segmentsLayout);
	segmentsLayout->addWidget(new QLabel("Segments", segmentsRow));
	m_segmentsField = new QSpinBox(segmentsRow);
	m_segmentsField->setRange(1, 10000);
	m_segmentsField->setValue(splineRenderer->GetSegments());
	segmentsLayout->addWidget(m_segmentsField);
	layout->addWidget(segmentsRow);
	layout->setAlignment(segmentsRow, Qt::AlignTop | Qt::AlignLeft);

	m_tileFactorField = new QFloatField(this);
	m_tileFactorField->setLabelText("Tile Factor");
	m_tileFactorField->setValue(splineRenderer->GetTileFactor());
	layout->addWidget(m_tileFactorField);
	layout->setAlignment(m_tileFactorField, Qt::AlignTop | Qt::AlignLeft);

	// Material field with Rasterization filter
	m_materialField = new QMaterialField(this, "Material",
		LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	m_materialField->SetMaterial(splineRenderer->GetMaterial());
	layout->addWidget(m_materialField);
	layout->setAlignment(m_materialField, Qt::AlignTop | Qt::AlignLeft);

	// --- Connect signals ---

	auto onPointChanged = [this](const LuxonEngine::Vector3&) {
		auto& curve = m_splineRenderer->GetCurve();
		curve.m_point1 = m_point1Field->value();
		curve.m_point2 = m_point2Field->value();
		curve.m_point3 = m_point3Field->value();
		m_splineRenderer->SetDirty();
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		};

	connect(m_point1Field, &QVector3Field::ValueChanged, this, onPointChanged);
	connect(m_point2Field, &QVector3Field::ValueChanged, this, onPointChanged);
	connect(m_point3Field, &QVector3Field::ValueChanged, this, onPointChanged);

	connect(m_widthField, &QFloatField::ValueChanged, this, [this](float value) {
		m_splineRenderer->SetWidth(value);
		m_splineRenderer->SetDirty();
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_segmentsField, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
		m_splineRenderer->SetSegments(value);
		m_splineRenderer->SetDirty();
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_tileFactorField, &QFloatField::ValueChanged, this, [this](float value) {
		m_splineRenderer->SetTileFactor(value);
		m_splineRenderer->SetDirty();
		LuxonEditor::EngineApplication::GetSceneManager()->RequestRender();
		});

	connect(m_materialField, &QMaterialField::ValueChanged, this, [this](ref<LuxonEngine::Rendering::Material> material) {
		m_splineRenderer->SetMaterial(material);
		});
}

SplineRendererWidget::~SplineRendererWidget()
{
}