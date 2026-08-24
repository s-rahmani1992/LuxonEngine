#pragma once

#include <QDialog>
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

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
	bool eventFilter(QObject* obj, QEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
private slots:
	void onRenderTick();

private:
	void initializeContext();

	ref<Scene> m_scene;
	ref<LuxonEngine::Rendering::GraphicContext> m_context;
	ref<LuxonEngine::Platform::GraphicWindow> m_window;

	QWidget* m_renderSurface = nullptr;
	QTimer* m_renderTimer = nullptr;
	QElapsedTimer m_frameTimer;

	int m_renderMode = 0;
};