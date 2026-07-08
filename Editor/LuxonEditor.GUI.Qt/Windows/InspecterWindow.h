#pragma once

#include <QWidget>
#include "ui_InspecterWindow.h"
#include <Core/SerializationStream.h>

namespace LuxonEditor::GUI::QT
{
	class InspecterWindow : public QWidget
	{
		Q_OBJECT

	public:
		InspecterWindow(QWidget *parent = nullptr);
		~InspecterWindow();

	private:
		Ui::InspecterWindowClass ui;
		QWidget* m_currentWidget = nullptr;
		std::string m_metaPath;
		LuxonEngine::SerializationStream m_stream;
		LuxonEngine::SerializationStream m_dataStream;
	};
}

