#pragma once

#include <QtWidgets/QWidget>
#include "ui_SceneEditorWindow.h"
#include <LuxonEditorAPI.h>

namespace LuxonEditor::GUI::QT {

	class SceneEditorWindow : public QWidget
	{
		Q_OBJECT

	public:
		SceneEditorWindow(QWidget* parent = nullptr);
		~SceneEditorWindow();

	protected:
		void resizeEvent(QResizeEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;
		void paintEvent(QPaintEvent* event) override;

	private:
		void OnSceneLoaded(ref<LuxonEngine::Scene> scene);
		void PrepareContextForScene();

		Ui::SceneEditorWindowClass ui;

		ref<LuxonEngine::Scene> m_scene;

		ref<LuxonEngine::Rendering::GraphicContext> m_context;
		ref<PerspectiveCamera> m_editorCamera;
		ref<LuxonEngine::Platform::GraphicWindow> m_window;

		bool m_isMoveMode = false;
		QPointF m_lastMousePos;

		size_t m_sceneLoadedCallbackId = 0;
	};
}