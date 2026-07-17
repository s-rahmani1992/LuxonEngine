#pragma once

#include <QWidget>
#include "ui_SplashStartWidget.h"
#include <QPixmap>
#include <QPainter>

namespace LuxonEditor::GUI::QT {
	class SplashStartWidget : public QWidget
	{
		Q_OBJECT

	public:
		SplashStartWidget(QWidget* parent = nullptr);
		~SplashStartWidget();
		void SetProgress(int value, const std::string& status) {
			ui.progressBar->setValue(value);
			ui.statusLabel->setText(QString::fromStdString(status));
		}

	protected:
		void paintEvent(QPaintEvent*) override;


	private:
		Ui::SplashStartWidgetClass ui;
		QPixmap pix;
	};
}
