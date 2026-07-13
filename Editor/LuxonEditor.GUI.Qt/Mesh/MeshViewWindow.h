#pragma once

#include <QDialog>
#include "ui_MeshViewWindow.h"

#include <LuxonEditorAPI.h>
#include <EngineAPI.h>

namespace LuxonEngine {
	class SerializationStream;
}

using namespace LuxonEngine;
using namespace LuxonEngine::Rendering;

class MeshViewWindow : public QDialog
{
	Q_OBJECT

public:
	MeshViewWindow(QWidget *parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
	~MeshViewWindow();

protected:
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
private:
	Ui::MeshViewWindowClass ui;

	ref<Scene> m_scene;
	SerializationStream* m_stream;
	ref<LuxonEngine::Rendering::GraphicContext> m_context;
	ref<LuxonEngine::Platform::GraphicWindow> m_window;
	ref<Mesh> m_mesh;

	bool m_isMoveMode = false;
	QPointF m_lastMousePos;
};

