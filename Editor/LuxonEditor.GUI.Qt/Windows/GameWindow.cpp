#include "GameWindow.h"

#include <QResizeEvent>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <qmessagebox.h>

// Wrap the dangerous call in a standalone function using SEH
static bool TryRender(const ref<Rendering::GraphicContext>& context)
{
	__try
	{
		context->Render();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

static bool TryPrepareScene(const ref<Rendering::GraphicContext>& context, const ref<Scene>& scene)
{
	__try
	{
		context->PrepareScene(scene);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

GameWindow::GameWindow(QWidget* parent, ref<Scene> scene, int renderMode)
	: QDialog(parent), m_scene(scene), m_renderMode(renderMode)
{
	auto title = QString("Game - %1").arg(m_renderMode == 0 ? "Hybrid" : "Ray Tracing");
	setWindowTitle(title);
	setFixedSize(1280, 720);

	m_renderSurface = new QWidget(this);
	m_renderSurface->setAttribute(Qt::WA_NativeWindow);
	m_renderSurface->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_renderSurface);
	setLayout(layout);

	m_renderSurface->installEventFilter(this);

	setFocusPolicy(Qt::StrongFocus);

	m_renderTimer = new QTimer(this);
	m_renderTimer->setInterval(1000 / 60); // ~60 fps
	connect(m_renderTimer, &QTimer::timeout, this, &GameWindow::onRenderTick);
}

GameWindow::~GameWindow()
{
	m_renderTimer->stop();
	if (m_context)
		m_context->Flush();
}

void GameWindow::initializeContext()
{
	HWND h = (HWND)m_renderSurface->winId();

	LuxonEngine::Platform::WindowProperties props{
		.width = (UInt16)m_renderSurface->width(),
		.height = (UInt16)m_renderSurface->height(),
	};

	m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);

	if(m_renderMode == 0)
	{
		m_context = GetGPUApplication()->CreateHybridContextForWindows(m_window);
	}
	else
	{
		m_context = GetGPUApplication()->CreateRayTracingContextForWindows(m_window);
	}

	if (!TryPrepareScene(m_context, m_scene))
	{
		Logger::LogError("GameWindow::initializeContext: Access violation during scene preparation.");
		QMessageBox::critical(this, "Rendering Error", "Fatal error while preparing the scene.");
		QTimer::singleShot(0, this, &QWidget::close);
		return;
	}
	m_renderTimer->start();
}

void GameWindow::onRenderTick()
{
	update();
}

void GameWindow::paintEvent(QPaintEvent* event)
{
	if (m_context == nullptr || m_scene == nullptr)
		return;

	if (!TryRender(m_context))
	{
		Logger::LogError("GameWindow::paintEvent: Access violation during rendering.");
		m_renderTimer->stop();
		QTimer::singleShot(0, this, &QWidget::close);
	}
}

void GameWindow::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::MouseButton::RightButton)
		return;

	QPoint localPos = m_renderSurface->mapFromGlobal(event->globalPosition().toPoint());
	if (m_renderSurface->rect().contains(localPos))
	{
		m_isMoveMode = true;
		m_lastMousePos = event->position();
	}
}

void GameWindow::mouseReleaseEvent(QMouseEvent* event)
{
	m_isMoveMode = false;
}

void GameWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isMoveMode)
	{
		float deltaX = event->position().x() - m_lastMousePos.x();
		float deltaY = event->position().y() - m_lastMousePos.y();
		auto cameraTransform = m_scene->mainCamera->GetTransform();
		cameraTransform->RotateAround(cameraTransform->Up(), -deltaX * 0.05f);
		cameraTransform->RotateAround(cameraTransform->Right(), -deltaY * 0.05f);
		m_lastMousePos = event->position();
		update();
	}
}

void GameWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_W)
	{
		m_scene->mainCamera->GetTransform()->MoveForward(0.05f);
		update();
	}
	else if (event->key() == Qt::Key_S)
	{
		m_scene->mainCamera->GetTransform()->MoveForward(-0.05f);
		update();
	}
	if (event->key() == Qt::Key_A)
	{
		m_scene->mainCamera->GetTransform()->MoveRight(-0.05f);
		update();
	}
	else if (event->key() == Qt::Key_D)
	{
		m_scene->mainCamera->GetTransform()->MoveRight(0.05f);
		update();
	}
}

bool GameWindow::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_renderSurface && event->type() == QEvent::Resize)
	{
		if (m_context == nullptr)
			initializeContext();
	}
	return false;
}