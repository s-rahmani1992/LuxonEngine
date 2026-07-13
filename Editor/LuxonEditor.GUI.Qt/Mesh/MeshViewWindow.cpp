#include "MeshViewWindow.h"
#include <Core/SerializationStream.h>

#include <QResizeEvent>
#include <QSize>
#include <StringUtilities.h>

MeshViewWindow::MeshViewWindow(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QDialog(parent), m_stream(stream)
{
	ui.setupUi(this);
	ui.context->setAttribute(Qt::WA_NativeWindow);
	ui.panel->layout()->setAlignment(ui.vectexLabel, Qt::AlignTop);
	ui.panel->layout()->setAlignment(ui.triangleLabel, Qt::AlignTop);
	layout()->setAlignment(ui.panel, Qt::AlignTop);

	auto id = m_stream->GetGuid("uuid");
	m_mesh = GetAssetManager()->GetMesh(id);

	if (m_mesh == nullptr)
		m_mesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

	ui.vectexLabel->setText("Vertex Count: " + QString::number(m_mesh->GetVertexCount()));
	ui.triangleLabel->setText("Triangle Count: " + QString::number(m_mesh->GetIndexCount() / 3));
}

MeshViewWindow::~MeshViewWindow()
{
	if (m_context)
		m_context->Flush();
}

void MeshViewWindow::resizeEvent(QResizeEvent * event)
{
	auto size = ui.context->size();

	if (m_context == nullptr) {
		HWND h = (HWND)ui.context->winId();
		LuxonEngine::Platform::WindowProperties props{
			.width = (UInt16)size.width(),
			.height = (UInt16)size.height(),
		};

		m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);
		m_context = GetGPUApplication()->CreateHybridContextForWindows(m_window);

		auto camtransform = std::make_shared<Transform>(Vector3(-5.2f, 1.9f, -1.1f), Vector3(1.0f), Vector3(-0.17f, -0.95f, 0.17f), 84);
		ref<PerspectiveCamera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)props.width / props.height, 45);

		std::wstring root = CharToString((GetProjectPath() + "/Data/InternalShaders/").c_str());
		std::string errorStr;
		auto shaderRegistery = GetGPUApplication()->CreateShaderRegistery();
		std::wstring simpleLightRasterPath = root + L"simple_color.hlsl";
		auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, errorStr);

		if (lightRasterProgram == nullptr) {
			errorStr = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + errorStr;
			return;
		}

		auto materialFactory = GetGPUApplication()->CreateMaterialFactory();

		auto meshMaterial = materialFactory->CreateMaterial(lightRasterProgram);
		meshMaterial->SetValue("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
		meshMaterial->SetValue("ambient", 0.2f);
		meshMaterial->SetValue("diffuse", 0.5f);
		meshMaterial->SetValue("specular", 0.7f);

		auto meshTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
		auto meshRenderer = std::make_shared<MeshRenderer>(m_mesh, meshMaterial);
		auto meshEntity = std::make_shared<LuxonEngine::GameEntity>(meshTransform, meshRenderer, nullptr);

		SceneLightData lightData;

		lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(2.0f, -6.0f, 2.0f),
			.intensity = 0.5f,
			});

		m_scene = std::make_shared<Scene>();
		m_scene->mainCamera = mainCamera;
		m_scene->lightData = lightData;
		m_scene->entities = { meshEntity };
		m_scene->behaviours = { };
		m_scene->rtGlobalMaterial = nullptr;
		m_context->PrepareScene(m_scene);
	}

	std::dynamic_pointer_cast<PerspectiveCamera>(m_scene->mainCamera)->ChangeAspect((float)size.width() / size.height());
	m_context->Render();
}

void MeshViewWindow::mousePressEvent(QMouseEvent* event)
{
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
	m_context->Render();
}
