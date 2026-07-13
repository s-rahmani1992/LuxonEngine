#pragma once

#include <QWidget>
#include "ui_Model3DInspecterWidget.h"

namespace LuxonEngine {
	class SerializationStream;
}

namespace LuxonEditor::GUI::QT {
	class Model3DInspecterWidget : public QWidget
	{
		Q_OBJECT

	public:
		Model3DInspecterWidget(QWidget* parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
		~Model3DInspecterWidget();

	private:
		Ui::Model3DInspecterWidgetClass ui;
		std::vector<LuxonEngine::SerializationStream> m_meshes;
	};
}
