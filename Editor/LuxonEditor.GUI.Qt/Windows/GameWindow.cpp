#include "GameWindow.h"

#include <QResizeEvent>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <qmessagebox.h>

// Wrap the dangerous call in a standalone function using SEH
static bool TryRender(const ref<Rendering::GraphicContext>& context)
{
#ifdef _DEBUG
	context->Render();
	return true;
#else
	__try
	{
		context->Render();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
#endif
}

static bool TryPrepareScene(const ref<Rendering::GraphicContext>& context, const ref<Scene>& scene)
{
#ifdef _DEBUG
	context->PrepareScene(scene);
	return true;
#else
	__try
	{
		context->PrepareScene(scene);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
#endif	
}

GameWindow::GameWindow(QWidget* parent, ref<Scene> scene, int renderMode)
	: QDialog(parent), m_scene(scene), m_renderMode(renderMode)
{
	auto title = QString("Game - %1").arg(m_renderMode == 1 ? "Hybrid" : "Ray Tracing");
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
	m_renderTimer->setTimerType(Qt::PreciseTimer);
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

	if(m_renderMode == 1)
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

	for(auto& behaviour : m_scene->behaviours)
	{
		behaviour->Start();
	}

	m_renderTimer->start();
}

void GameWindow::onRenderTick()
{
	if (m_context == nullptr || m_scene == nullptr)
		return;

	float deltaTime = 0.0f;
	if (m_frameTimer.isValid())
	{
		deltaTime = m_frameTimer.nsecsElapsed() / 1000000000.0f;
	}
	m_frameTimer.restart();

	for (auto& behaviour : m_scene->behaviours)
	{
		behaviour->Update(deltaTime);
	}


	if (!TryRender(m_context))
	{
		Logger::LogError("GameWindow::paintEvent: Access violation during rendering.");
		m_renderTimer->stop();
		QTimer::singleShot(0, this, &QWidget::close);
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

void GameWindow::closeEvent(QCloseEvent* event)
{
	m_renderTimer->stop();
}
