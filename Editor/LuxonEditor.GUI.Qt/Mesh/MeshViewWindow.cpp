#include "MeshViewWindow.h"
#include <Core/SerializationStream.h>

#include <QResizeEvent>
#include <QSize>
#include <StringUtilities.h>
#include "../Core/TransformWidget.h"
#include <QSplitter>
#include <LuxonEditorAPI.h>

MeshViewWindow::MeshViewWindow(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QDialog(parent), m_stream(stream)
{
	ui.setupUi(this);
	ui.context->setAttribute(Qt::WA_NativeWindow);
	ui.panel->layout()->setAlignment(ui.infoPanel, Qt::AlignTop);
	ui.panel->layout()->setAlignment(ui.transformPanel, Qt::AlignTop);
	ui.infoPanel->setStyleSheet(ui.infoPanel->styleSheet() + "#infoPanel { border: 1px solid #555555; border-radius: 5px; }");
	ui.transformPanel->setStyleSheet(ui.transformPanel->styleSheet() + "#transformPanel { border: 1px solid #555555; border-radius: 5px; }");

	m_transformWidget = new TransformWidget(ui.transformPanel);
	ui.transformPanel->layout()->addWidget(m_transformWidget);
	ui.transformPanel->layout()->setAlignment(m_transformWidget, Qt::AlignTop);
	m_transformWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	connect(m_transformWidget, &TransformWidget::ValueChanged, this, [this]() {
		update(); 
		});

	static_cast<QVBoxLayout*>(ui.panel->layout())->addStretch(1);

	auto id = m_stream->GetGuid("uuid");
	m_mesh = GetAssetManager()->GetMesh(id);

	if (m_mesh == nullptr)
		m_mesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

	ui.vectexLabel->setText("Vertex Count: " + QString::number(m_mesh->GetVertexCount()));
	ui.triangleLabel->setText("Triangle Count: " + QString::number(m_mesh->GetIndexCount() / 3));
	
	delete this->layout();

	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QBoxLayout* mainLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(splitter, 1);
	setLayout(mainLayout);
	splitter->setHandleWidth(2);
	splitter->setStyleSheet(splitter->styleSheet() +
		"QSplitter::handle { background: #444444 }"
		"QSplitter::handle:hover { background: #666666; }"
	);

	splitter->addWidget(ui.context);
	splitter->addWidget(ui.panel);
	splitter->setCollapsible(0, false);
	splitter->setCollapsible(1, false);
	ui.context->installEventFilter(this);
}

MeshViewWindow::~MeshViewWindow()
{
	if (m_context)
		m_context->Flush();
}

void MeshViewWindow::resizeEvent1(QResizeEvent * event)
{
	auto size = ui.context->size();

	if(size.width() <= 0 || size.height() <= 0)
		return;

	if (m_context == nullptr) {
		HWND h = (HWND)ui.context->winId();
		LuxonEngine::Platform::WindowProperties props{
			.width = (UInt16)size.width(),
			.height = (UInt16)size.height(),
		};

		m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);
		m_context = GetGPUApplication()->CreateHybridContextForWindows(m_window);

		auto camtransform = std::make_shared<Transform>(Vector3(-5.2f, 2.8f, 3.6f), Vector3(1.0f), Vector3(-0.06f, -0.72f, 0.11f), 121);
		ref<PerspectiveCamera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)props.width / props.height, 45);

		std::wstring root = CharToString((GetProjectPath() + "/Data/InternalShaders/").c_str());
		std::string errorStr;


		auto shaderRegistery = LuxonEditor::EngineApplication::GetShaderRegistery();
		auto guid = LuxonEditor::GuidGenerator::GenerateGUIDFromString("467ac325-1305-45bf-8088-f45f249077db");
		auto program = shaderRegistery->GetProgram(guid);
		auto lightRasterProgram = std::shared_ptr<LuxonEngine::Rendering::ShaderProgram>(program, [](LuxonEngine::Rendering::ShaderProgram*) {
			// do nothing
			});

		auto materialFactory = GetGPUApplication()->CreateMaterialFactory();

		auto meshMaterial = materialFactory->CreateMaterial(lightRasterProgram);
		meshMaterial->SetValue("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
		meshMaterial->SetValue("ambient", 0.2f);
		meshMaterial->SetValue("diffuse", 0.5f);
		meshMaterial->SetValue("specular", 0.7f);

		auto meshTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
		auto meshRenderer = std::make_shared<MeshRenderer>(m_mesh, meshMaterial);
		auto meshEntity = std::make_shared<LuxonEngine::GameEntity>(meshTransform, meshRenderer, nullptr);

		auto planeMesh = ShapeBuilder::CreatePlane(5.0f, 5.0f, 25, 25);

		auto textureProgram = shaderRegistery->GetProgram(LuxonEditor::GuidGenerator::GenerateGUIDFromString("e0086dc4-3a59-4d7b-ae27-69f04a52b154"));
		auto textureProgramRef = std::shared_ptr<LuxonEngine::Rendering::ShaderProgram>(textureProgram, [](LuxonEngine::Rendering::ShaderProgram*) {
			// do nothing
			});
		
		auto gridTex = GetAssetManager()->GetTexture(LuxonEditor::GuidGenerator::GenerateGUIDFromString("c51c0339-7e57-42e6-901d-bc634b212166"));

		auto groundMaterial = materialFactory->CreateMaterial(textureProgramRef);
		groundMaterial->SetTexture2D("mainTexture", gridTex);
		groundMaterial->SetValue("ambient", 0.1f);
		groundMaterial->SetValue("diffuse", 0.8f);
		groundMaterial->SetValue("specular", 0.4f);
		auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
		auto groundMeshRenderer = std::make_shared<MeshRenderer>(planeMesh, groundMaterial);
		auto groundEntity = std::make_shared<LuxonEngine::GameEntity>(groundTransform, groundMeshRenderer, nullptr);


		SceneLightData lightData;

		lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(2.0f, -6.0f, 2.0f),
			.intensity = 0.5f,
			});

		m_scene = std::make_shared<Scene>();
		m_scene->mainCamera = mainCamera;
		m_scene->lightData = lightData;
		m_scene->entities = { meshEntity, groundEntity };
		m_scene->behaviours = { };
		m_scene->rtGlobalMaterial = nullptr;
		m_context->PrepareScene(m_scene);
		m_transformWidget->SetTransform(meshTransform);
	}
	else {
		m_context->Resize(size.width(), size.height());
		std::dynamic_pointer_cast<PerspectiveCamera>(m_scene->mainCamera)->ChangeAspect((float)size.width() / size.height());
	}

	update();
}

void MeshViewWindow::mousePressEvent(QMouseEvent* event)
{
	if(event->button() != Qt::MouseButton::RightButton)
		return;

	QPoint localPos = ui.context->mapFromGlobal(event->globalPosition().toPoint());
	if (ui.context->rect().contains(localPos))
	{
		m_isMoveMode = true;
		m_lastMousePos = event->position();
	}
}

void MeshViewWindow::mouseReleaseEvent(QMouseEvent* event)
{
	m_isMoveMode = false;
}

void MeshViewWindow::mouseMoveEvent(QMouseEvent* event)
{
	if(m_isMoveMode)
	{
		// Mouse is inside ui.context widget
		float deltaX = event->position().x() - m_lastMousePos.x();
		float deltaY = event->position().y() - m_lastMousePos.y();
		auto cameraTransform = m_scene->mainCamera->GetTransform();
		cameraTransform->RotateAround(cameraTransform->Up(), -deltaX * 0.05f);
		cameraTransform->RotateAround(cameraTransform->Right(), -deltaY * 0.05f);
		m_lastMousePos = event->position();
		update();
	}
}

void MeshViewWindow::keyPressEvent(QKeyEvent* event)
{
	if(event->key() == Qt::Key_W)
	{
		m_scene->mainCamera->GetTransform()->MoveForward(0.1f);
		update();
	}
	else if(event->key() == Qt::Key_S)
	{
		m_scene->mainCamera->GetTransform()->MoveForward(-0.1f);
		update();
	}
	if(event->key() == Qt::Key_A)
	{
		m_scene->mainCamera->GetTransform()->MoveRight(-0.1f);
		update();
	}
	else if(event->key() == Qt::Key_D)
	{
		m_scene->mainCamera->GetTransform()->MoveRight(0.1f);
		update();
	}
}

void MeshViewWindow::paintEvent(QPaintEvent* event)
{
	if(m_context == nullptr || m_scene == nullptr)
		return;
	m_context->Render();
}

bool MeshViewWindow::eventFilter(QObject* widget, QEvent* event)
{
	if(widget == ui.context)
	{
		if(event->type() == QEvent::Resize)
		{
			QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
			resizeEvent1(resizeEvent);
		}
	}
	return false;
}
