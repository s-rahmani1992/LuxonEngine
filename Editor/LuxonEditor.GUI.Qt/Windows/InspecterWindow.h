#pragma once

#include <QWidget>
#include "ui_InspecterWindow.h"

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
	};
}

