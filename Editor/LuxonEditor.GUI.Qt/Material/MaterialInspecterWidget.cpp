#include "MaterialInspecterWidget.h"
#include <Core/SerializationStream.h>
#include <Widgets/QFloatField.h>
#include "../Texture/QTextureField.h"
#include "../Texture/QColorField.h"
#include <QResizeEvent>
#include <QSize>
#include <Core/MaterialImporter.h>
#include <qsplitter.h>
#include <LuxonEditorAPI.h>

MaterialInspecterWidget::MaterialInspecterWidget(QWidget* parent, LuxonEngine::SerializationStream* stream, std::string path)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.context->setAttribute(Qt::WA_NativeWindow);
	ui.scrollArea->setWidget(ui.dataFields);
	ui.scrollArea->setStyleSheet(ui.scrollArea->styleSheet() + "#scrollArea { padding-left: 6px; border-radius: 6px; border: 1px solid #f1f1f1; }");
	
	ui.saveButton->setStyleSheet(ui.saveButton->styleSheet() + "QPushButton { margin: 8px; padding: 4px 8px; }");
	ui.uiSection->layout()->setAlignment(ui.saveButton, Qt::AlignHCenter);
	QSplitter* splitter = new QSplitter(Qt::Vertical, this);
	splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	auto mainLayout = static_cast<QBoxLayout*>(layout());
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(ui.programLabel);
	mainLayout->addWidget(splitter, 1);
	setLayout(mainLayout);
	splitter->setHandleWidth(2);
	splitter->setStyleSheet(splitter->styleSheet() +
		"QSplitter::handle { background: #444444 }"
		"QSplitter::handle:hover { background: #666666; }"
	);

	splitter->addWidget(ui.uiSection);
	splitter->addWidget(ui.context);
	splitter->setCollapsible(0, false);
	splitter->setCollapsible(1, false);

	ui.context->installEventFilter(this);

	auto guid = stream->GetGuid("uuid");
	m_material = GetAssetManager()->GetMaterial(guid);

	CreateScene(ui.context->size());
	UpdateDataFields();
	
	layout()->setAlignment(ui.saveButton, Qt::AlignHCenter);

	connect(ui.saveButton, &QPushButton::clicked, this, [this, path]() {
		LuxonEngine::SerializationStream stream;
		LuxonEditor::MaterialImporter::SerializeMaterial(m_material, stream);
		stream.SaveToFile(path);
		});

	m_meshDeletedCallbackId = GetAssetManager()->RegisterMaterialChangedCallback([this](const ref<LuxonEngine::Rendering::Material>& material) {
		if(material != m_material)
			return;
		
		QMetaObject::invokeMethod(this, [this]() {
			UpdateDataFields();
			UpdateContext();
			UpdateContextUI();
			}, Qt::QueuedConnection);
		});
}

MaterialInspecterWidget::~MaterialInspecterWidget()
{
	if (m_context)
		m_context->Flush();

	GetAssetManager()->UnregisterMaterialChangedCallback(m_meshDeletedCallbackId);
}

bool MaterialInspecterWidget::eventFilter(QObject* widget, QEvent* event)
{
	if (widget == ui.context)
	{
		if (event->type() == QEvent::Resize)
		{
			QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
			auto& size = resizeEvent->size();

			if(m_window == nullptr) {
				HWND h = (HWND)ui.context->winId();
				LuxonEngine::Platform::WindowProperties props{
					.width = (UInt16)(size.width() * 10),
					.height = (UInt16)(size.height() * 10),
				};

				m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);

				UpdateContextUI();
				UpdateContext();
			}
			else {
				if (m_context != nullptr)
					m_context->Resize(size.width(), size.height());
				std::dynamic_pointer_cast<PerspectiveCamera>(m_scene->mainCamera)->ChangeAspect((float)size.width() / size.height());
			}

			if (m_context != nullptr)
				m_context->Render();
		}
	}
	return false;
}

void MaterialInspecterWidget::CreateScene(const QSize& size)
{
	auto camtransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 7.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 180);
	ref<PerspectiveCamera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)size.width() / size.height(), 45);

	ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

	auto sphereTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.5f), Vector3(1.0f, 0.0f, 0.0f), 90);
	m_renderer = std::make_shared<LuxonEngine::Rendering::MeshRenderer>(sphereMesh, m_material);
	auto sphereEntity = std::make_shared<LuxonEngine::GameEntity>(sphereTransform, m_renderer, nullptr);

	SceneLightData lightData;

	lightData.directionalLights.push_back(DirectionalLight{
		.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
		.direction = Vector3(-2.0f, 0.0f, -2.0f),
		.intensity = 0.5f,
		});

	m_scene = std::make_shared<Scene>();
	m_scene->mainCamera = mainCamera;
	m_scene->lightData = lightData;
	m_scene->entities = { sphereEntity };
	m_scene->behaviours = { };
	m_scene->rtGlobalMaterial = nullptr;
	m_scene->hybridBackgroundColor = Color(0.3f, 0.3f, 0.3f, 1.0f);
}

void MaterialInspecterWidget::UpdateContext()
{
	auto program = m_material->GetProgram();

	bool shouldCreateContext = (program == nullptr) || (program->GetType() == LuxonEngine::Rendering::ShaderProgramType::Rasterization);

	if (m_context != nullptr) {
		m_context->Flush();
		m_context->Detach();
		m_context = nullptr;
	}

	if (shouldCreateContext) {
		auto defaultMaterial = GetAssetManager()->GetFallbackMaterial();
		m_renderer->SetMaterial(program == nullptr ? defaultMaterial : m_material);
		m_context = LuxonEditor::EngineApplication::GetGPUApplication()->CreateHybridContextForWindows(m_window);
		m_context->PrepareScene(m_scene);
		m_context->Render();
		return;
	}
}

void MaterialInspecterWidget::UpdateContextUI()
{
	auto program = m_material->GetProgram();

	if (program == nullptr) {
		ui.errorLabel->setVisible(false);
		return;
	}

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
}

void MaterialInspecterWidget::UpdateDataFields()
{

	QLayoutItem* item;
	while ((item = ui.dataFields->layout()->takeAt(0)) != nullptr) {
		if (item->widget()) {
			delete item->widget();   // deletes the actual child widget
		}
		delete item;                 // deletes the layout item wrapper
	}

	auto program = m_material->GetProgram();
	auto shaderEntry = LuxonEditor::EngineApplication::GetShaderRegistery()->GetShaderEntry(m_material->GetProgramGuid());

	if (program == nullptr) {
		QLabel* errorLabel = new QLabel(ui.dataFields);
		
		QString errorMessage;

		if(shaderEntry == nullptr) {
			errorMessage = "This material's shader program does not exist";
			ui.programLabel->setText("Missing Shader");
		}
		else {
			errorMessage = QString::fromStdString("This material's shader program has compilation error:\n" + shaderEntry->compileError);
			ui.programLabel->setText(QString::fromStdString("Program: " + shaderEntry->name));
		}
		errorLabel->setText(errorMessage);
		ui.dataFields->layout()->addWidget(errorLabel);
		ui.dataFields->layout()->setAlignment(errorLabel, Qt::AlignTop);
		ui.saveButton->setEnabled(false);
		return;
	}

	ui.programLabel->setText(QString::fromStdString("Program: " + shaderEntry->name));
	auto valueFields = m_material->GetValueFields();

	for (auto& [fieldName, valueData] : *valueFields) {
		if (fieldName[0] == '_') {
			continue; // Skip private fields
		}

		switch (valueData.size) {
			case sizeof(Int32) :
			{
				auto floatfield = new QFloatField(ui.dataFields);
				floatfield->setLabelText(QString::fromStdString(fieldName));
				floatfield->setValue(m_material->GetValue<float>(fieldName, 0.0f));
				ui.dataFields->layout()->addWidget(floatfield);
				ui.dataFields->layout()->setAlignment(floatfield, Qt::AlignTop);
				connect(floatfield, &QFloatField::ValueChanged, this, [this, fieldName](float newValue) {
					m_material->SetValue(fieldName, newValue);
					if (m_context)
						m_context->Render();
					});
				break;
			}
			case sizeof(Color) :
			{
				auto colorField = new QColorField(ui.dataFields, fieldName);
				colorField->SetColor(m_material->GetValue<Color>(fieldName, Color()));
				ui.dataFields->layout()->addWidget(colorField);
				ui.dataFields->layout()->setAlignment(colorField, Qt::AlignTop);
				connect(colorField, &QColorField::ValueChanged, this, [this, fieldName](Color newColor) {
					m_material->SetValue(fieldName, newColor);
					if (m_context)
						m_context->Render();
					});
				break;
			}
			default:
				// Handle other sizes if necessary
				break;
		}
	}

	auto textureFields = m_material->GetTextureFields();

	for (auto& [fieldName, textureData] : *textureFields) {
		if (fieldName[0] == '_') {
			continue; // Skip private fields
		}
		auto textureField = new QTextureField(ui.dataFields, fieldName);
		ui.dataFields->layout()->addWidget(textureField);
		ui.dataFields->layout()->setAlignment(textureField, Qt::AlignTop);
		textureField->SetTexture(textureData.texture);
		connect(textureField, &QTextureField::ValueChanged, this, [this, fieldName](ref<Texture2D> newTexture) {
			m_material->SetTexture2D(fieldName, newTexture);
			if (m_context)
				m_context->Render();
			});
	}

	ui.saveButton->setEnabled(true);
}

