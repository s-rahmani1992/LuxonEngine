#pragma once

#include <QWidget>
#include "ui_Model3DInspecterWidget.h"
#include <Core/SerializationStream.h>

class QVector3Field;
class QFloatField;

namespace LuxonEngine {
	class SerializationStream;
}

namespace LuxonEditor::GUI::QT {
	class MeshItem;

	class Model3DInspecterWidget : public QWidget
	{
		Q_OBJECT

	public:
		Model3DInspecterWidget(QWidget* parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
		~Model3DInspecterWidget();

	Q_SIGNALS:
		void PropertyUpdated(LuxonEngine::SerializationStream* stream);

	private:
		Ui::Model3DInspecterWidgetClass ui;
		std::vector<LuxonEngine::SerializationStream> m_meshes;
		std::vector<MeshItem*> m_meshItems;
		QVector3Field* m_positionField;
		QFloatField* m_rotationAngleField;
		QVector3Field* m_rotationAxisField;
		QVector3Field* m_scaleField;

		LuxonEngine::SerializationStream m_stream;
	};
}
