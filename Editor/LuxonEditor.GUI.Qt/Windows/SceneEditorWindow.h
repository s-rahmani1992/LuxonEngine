#pragma once

#include <QWidget>
#include "ui_SceneEditorWindow.h"
#include <LuxonEditorAPI.h>
#include <EngineAPI.h>

using namespace LuxonEngine;
using namespace LuxonEngine::Rendering;

namespace LuxonEngine::Rendering {
	class GraphicContext;
}

namespace LuxonEditor::GUI::QT {
	class SceneEditorWindow : public QWidget
	{
		Q_OBJECT

	public:
		SceneEditorWindow(QWidget* parent = nullptr);
		~SceneEditorWindow();

	protected:
		void resizeEvent(QResizeEvent* event) override;

	private:
		Ui::SceneEditorWindowClass ui;

		ref<LuxonEngine::Platform::GraphicWindow> m_window;
		ref<LuxonEngine::Rendering::GraphicContext> m_context;

		ref<Scene> m_scene;
	};
}
