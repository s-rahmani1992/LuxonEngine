#pragma once

#include <QDialog>
#include <QWidget>
#include <QTimer>

#include <LuxonEditorAPI.h>
#include <EngineAPI.h>

using namespace LuxonEngine;
using namespace LuxonEngine::Rendering;

class GameWindow : public QDialog
{
	Q_OBJECT

public:
	GameWindow(QWidget* parent, ref<Scene> scene, int renderMode = 0);
	~GameWindow();

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
	void onRenderTick();

private:
	void initializeContext();

	ref<Scene> m_scene;
	ref<LuxonEngine::Rendering::GraphicContext> m_context;
	ref<LuxonEngine::Platform::GraphicWindow> m_window;

	QWidget* m_renderSurface = nullptr;
	QTimer* m_renderTimer = nullptr;

	bool m_isMoveMode = false;
	QPointF m_lastMousePos;
	int m_renderMode = 0;
};