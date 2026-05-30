#pragma once

#include <QWidget>
#include "ui_ConsoleLogWindow.h"

namespace LuxonEditor::GUI::QT {
	class LogModel;
	class LogFilter;

	class ConsoleLogWindow : public QWidget
	{
		Q_OBJECT

	public:
		ConsoleLogWindow(QWidget* parent = nullptr);
		~ConsoleLogWindow();

	private:
		Ui::ConsoleLogWindowClass ui;
		LogModel* m_logModel;
		LogFilter* m_filter;
	};
}
