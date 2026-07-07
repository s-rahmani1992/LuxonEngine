#include "ShaderInspecterWidget.h"
#include "ShaderInspecterWidget.h"
#include <Core/EngineShaderRegistry.h>
#include <Core/SerializationStream.h>
#include <EngineAPI.h>

ShaderInspecterWidget::ShaderInspecterWidget(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent)
{
	ui.setupUi(this);
	layout()->setAlignment(ui.propertiesContainer, Qt::AlignTop);

	if (stream == nullptr) {
		return;
		//auto dataNode = stream->Object("data");
		
	}
	LuxonEditor::EngineShaderRegistry::FillProperties(m_currentProperties, *stream);
	m_newProperties = m_currentProperties;


	ui.shaderTypeBox->setCurrentIndex((int)m_newProperties.type);

	OnShaderTypeChanged(m_newProperties.type);

	connect(ui.shaderTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
		LuxonEngine::Rendering::ShaderProgramType type = (LuxonEngine::Rendering::ShaderProgramType)index;
		OnShaderTypeChanged(type);
	});

	bool isRasterization = m_newProperties.type == LuxonEngine::Rendering::ShaderProgramType::Rasterization;
	ui.vertexField->InputText()->setText(isRasterization ? m_newProperties.rasterProperties.vertexMain : "vs_main");
	
	bool isGeometryActive = isRasterization && m_newProperties.rasterProperties.geometryMain != nullptr;
	ui.geometryField->Toggle(isGeometryActive);
	ui.geometryField->InputText()->setText(isGeometryActive ? m_newProperties.rasterProperties.geometryMain : "gs_main");
	
	ui.pixelField->InputText()->setText(isRasterization ? m_newProperties.rasterProperties.pixelMain : "ps_main");

	bool isRayTracing = m_newProperties.type == LuxonEngine::Rendering::ShaderProgramType::RayTracing;
	
	bool isRayGenActive = isRayTracing && m_newProperties.rayTracingProperties.rayGen != nullptr;
	ui.rayGenField->Toggle(isRayGenActive);
	ui.rayGenField->InputText()->setText(isRayGenActive ? m_newProperties.rayTracingProperties.rayGen : "rayGen");
	
	bool isClosestHitActive = isRayTracing && m_newProperties.rayTracingProperties.closestHit != nullptr;
	ui.closestHitField->Toggle(isClosestHitActive);
	ui.closestHitField->InputText()->setText(isClosestHitActive ? m_newProperties.rayTracingProperties.closestHit : "chs");
	
	bool isMissActive = isRayTracing && m_newProperties.rayTracingProperties.miss != nullptr;
	ui.missField->Toggle(isMissActive);
	ui.missField->InputText()->setText(isMissActive ? m_newProperties.rayTracingProperties.miss : "miss");
	
	bool isCompute = m_newProperties.type == LuxonEngine::Rendering::ShaderProgramType::Compute;
	ui.computeMain->InputText()->setText(isCompute ? QString::fromUtf8(m_newProperties.computeProperties.computeMain) : "cs_main");
}

ShaderInspecterWidget::~ShaderInspecterWidget()
{
}

void ShaderInspecterWidget::OnShaderTypeChanged(LuxonEngine::Rendering::ShaderProgramType programType)
{
	m_newProperties.type = programType;

	ui.rasterizationContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Rasterization);
	ui.rayTracingContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::RayTracing);
	ui.computeContainer->setVisible(programType == LuxonEngine::Rendering::ShaderProgramType::Compute);
}

