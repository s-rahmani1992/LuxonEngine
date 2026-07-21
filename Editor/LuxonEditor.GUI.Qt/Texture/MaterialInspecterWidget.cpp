#include "MaterialInspecterWidget.h"
#include <Core/SerializationStream.h>
#include <Widgets/QFloatField.h>
#include "../Texture/QTextureField.h"
#include "../Texture/QColorField.h"
#include <QResizeEvent>
#include <QSize>

MaterialInspecterWidget::MaterialInspecterWidget(QWidget* parent, LuxonEngine::SerializationStream* stream, std::string path)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.context->setAttribute(Qt::WA_NativeWindow);
	layout()->setAlignment(ui.dataFields, Qt::AlignTop);
	auto guid = stream->GetGuid("uuid");
	m_material = GetAssetManager()->GetMaterial(guid);

	auto programType = m_material->GetProgram()->GetType();

	switch (programType) {
	case LuxonEngine::Rendering::ShaderProgramType::Rasterization:
		ui.errorLabel->setVisible(false);
		break;
	case LuxonEngine::Rendering::ShaderProgramType::RayTracing:
		ui.errorLabel->setText("Preview for Ray Tracing Materials is not supported at the moment");
		ui.errorLabel->setVisible(true);
		break;
	case LuxonEngine::Rendering::ShaderProgramType::Compute:
		ui.errorLabel->setText("Preview for Compute Materials is not supported at the moment");
		ui.errorLabel->setVisible(true);
		break;
	}

	auto valueFields = m_material->GetValueFields();

	for(auto& [fieldName, valueData] : *valueFields) {
		if(fieldName[0] == '_') {
			continue; // Skip private fields
		}

		switch(valueData.size) {
			case sizeof(Int32) : 
			{
				auto floatfield = new QFloatField(ui.dataFields);
				floatfield->setLabelText(QString::fromStdString(fieldName));
				floatfield->setValue(m_material->GetValue<float>(fieldName, 0.0f));
				ui.dataFields->layout()->addWidget(floatfield);
				ui.dataFields->layout()->setAlignment(floatfield, Qt::AlignTop);
				break; 
			}
			case sizeof(Color):
			{
				auto colorField = new QColorField(ui.dataFields, fieldName);
				colorField->SetColor(m_material->GetValue<Color>(fieldName, Color()));
				ui.dataFields->layout()->addWidget(colorField);
				ui.dataFields->layout()->setAlignment(colorField, Qt::AlignTop);
				break;
			}
			default:
				// Handle other sizes if necessary
				break;
		}
	}

	auto textureFields = m_material->GetTextureFields();

	for(auto& [fieldName, textureData] : *textureFields) {
		if(fieldName[0] == '_') {
			continue; // Skip private fields
		}
		auto textureField = new QTextureField(ui.dataFields, fieldName);
		ui.dataFields->layout()->addWidget(textureField);
		ui.dataFields->layout()->setAlignment(textureField, Qt::AlignTop);
		textureField->SetTexture(textureData.texture);
	}
}

MaterialInspecterWidget::~MaterialInspecterWidget()
{
	if (m_context)
		m_context->Flush();
}

void MaterialInspecterWidget::resizeEvent(QResizeEvent * event)
{
	if(m_material->GetProgram()->GetType() != LuxonEngine::Rendering::ShaderProgramType::Rasterization) {
		return;
	}

	auto size = ui.context->size();

	if (m_context == nullptr) {
		HWND h = (HWND)ui.context->winId();
		LuxonEngine::Platform::WindowProperties props{
			.width = (UInt16)(size.width() * 10),
			.height = (UInt16)(size.height() * 10),
		};

		m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);
		m_context = GetGPUApplication()->CreateHybridContextForWindows(m_window);

		auto camtransform = std::make_shared<Transform>(Vector3(-5.2f, 1.9f, -1.1f), Vector3(1.0f), Vector3(-0.17f, -0.95f, 0.17f), 84);
		ref<PerspectiveCamera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)props.width / props.height, 45);

		ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

		auto sphereTransform = std::make_shared<Transform>(Vector3(0.2f, 0.0f, 0.0f), Vector3(2.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
		auto sphereGBufferRenderer = std::make_shared<LuxonEngine::Rendering::MeshRenderer>(sphereMesh, m_material);
		auto sphereEntity = std::make_shared<LuxonEngine::GameEntity>(sphereTransform, sphereGBufferRenderer, nullptr);

		SceneLightData lightData;

		lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(2.0f, -6.0f, 2.0f),
			.intensity = 0.5f,
			});

		m_scene = std::make_shared<Scene>();
		m_scene->mainCamera = mainCamera;
		m_scene->lightData = lightData;
		m_scene->entities = { sphereEntity };
		m_scene->behaviours = { };
		m_scene->rtGlobalMaterial = nullptr;
		m_context->PrepareScene(m_scene);
	}

	std::dynamic_pointer_cast<PerspectiveCamera>(m_scene->mainCamera)->ChangeAspect((float)size.width() / size.height());
	m_context->Render();
}

