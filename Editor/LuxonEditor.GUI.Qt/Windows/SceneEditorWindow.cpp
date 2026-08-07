#include "SceneEditorWindow.h"
#include <QResizeEvent>
#include <QSize>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <StringUtilities.h>

LuxonEditor::GUI::QT::SceneEditorWindow::SceneEditorWindow(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.contextWidget->setAttribute(Qt::WA_NativeWindow);
	setFocusPolicy(Qt::StrongFocus);
}

LuxonEditor::GUI::QT::SceneEditorWindow::~SceneEditorWindow()
{
	if (m_context)
		m_context->Flush();
}

void LuxonEditor::GUI::QT::SceneEditorWindow::resizeEvent(QResizeEvent* event)
{
	auto& size = event->size();

	if (m_context == nullptr) {
		HWND h = (HWND)ui.contextWidget->winId();
		LuxonEngine::Platform::WindowProperties props{
			.width = (UInt16)size.width(),
			.height = (UInt16)size.height(),
		};

		m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);

		m_context = GetGPUApplication()->CreateEditorContext(m_window);
		auto camtransform = std::make_shared<Transform>(Vector3(-5.2f, 1.9f, -1.1f), Vector3(1.0f), Vector3(-0.17f, -0.95f, 0.17f), 84);
		m_editorCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)props.width / props.height, 45);

		m_scene = EngineApplication::GetSceneManager()->GetCurrentScene();
		auto sceneCamera = m_scene->mainCamera;
		m_scene->mainCamera = m_editorCamera;

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

		auto globalMat = m_scene->rtGlobalMaterial;
		m_scene->rtGlobalMaterial = meshMaterial;
		m_context->PrepareScene(m_scene);
		m_scene->mainCamera = sceneCamera;
		m_scene->rtGlobalMaterial = globalMat;
	}

	m_editorCamera->ChangeAspect((float)size.width() / size.height());
	m_context->Render();
}

void LuxonEditor::GUI::QT::SceneEditorWindow::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::MouseButton::RightButton)
		return;

	QPoint localPos = ui.contextWidget->mapFromGlobal(event->globalPosition().toPoint());
	if (ui.contextWidget->rect().contains(localPos))
	{
		m_isMoveMode = true;
		m_lastMousePos = event->position();
	}
}

void LuxonEditor::GUI::QT::SceneEditorWindow::mouseReleaseEvent(QMouseEvent* event)
{
	m_isMoveMode = false;
}

void LuxonEditor::GUI::QT::SceneEditorWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isMoveMode)
	{
		float deltaX = event->position().x() - m_lastMousePos.x();
		float deltaY = event->position().y() - m_lastMousePos.y();
		auto cameraTransform = m_editorCamera->GetTransform();
		cameraTransform->RotateAround(cameraTransform->Up(), -deltaX * 0.05f);
		cameraTransform->RotateAround(cameraTransform->Right(), -deltaY * 0.05f);
		m_lastMousePos = event->position();
		update();
	}
}

void LuxonEditor::GUI::QT::SceneEditorWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_W)
	{
		m_editorCamera->GetTransform()->MoveForward(0.1f);
		update();
	}
	else if (event->key() == Qt::Key_S)
	{
		m_editorCamera->GetTransform()->MoveForward(-0.1f);
		update();
	}
	if (event->key() == Qt::Key_A)
	{
		m_editorCamera->GetTransform()->MoveRight(-0.1f);
		update();
	}
	else if (event->key() == Qt::Key_D)
	{
		m_editorCamera->GetTransform()->MoveRight(0.1f);
		update();
	}
}

void LuxonEditor::GUI::QT::SceneEditorWindow::paintEvent(QPaintEvent* event)
{
	if (m_context == nullptr || m_scene == nullptr)
		return;
	m_context->Render();
}