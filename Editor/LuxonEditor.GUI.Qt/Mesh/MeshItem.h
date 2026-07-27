#pragma once

#include <QWidget>
#include "ui_MeshItem.h"

namespace LuxonEngine {
	class SerializationStream;
}

namespace LuxonEditor::GUI::QT {
	class MeshItem : public QWidget
	{
		Q_OBJECT

	public:
		MeshItem(QWidget* parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
		~MeshItem();

	protected:
		void paintEvent(QPaintEvent* event) override;

	private:
		Ui::MeshItemClass ui;
	};
}
