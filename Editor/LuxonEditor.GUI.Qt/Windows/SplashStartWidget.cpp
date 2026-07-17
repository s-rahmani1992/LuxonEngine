#include "SplashStartWidget.h"

namespace LuxonEditor::GUI::QT {
	SplashStartWidget::SplashStartWidget(QWidget* parent)
		: QWidget(parent), pix(":/LuxonEditorWindow/splash.png")
	{
		ui.setupUi(this);
		setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint);
		setAttribute(Qt::WA_TranslucentBackground);

		ui.progressBar->setStyleSheet(
			"QProgressBar {"
			"  border: 1px solid grey;"
			"  border-radius: 4px;"
			"  text-align: center;"
			"}"
			"QProgressBar::chunk {"
			"  background-color: #4a90d9;"
			"  border-radius: 3px;"
			"}"
		);
	}

	SplashStartWidget::~SplashStartWidget()
	{
	}

	void SplashStartWidget::paintEvent(QPaintEvent*)
	{
		QPainter p(this);
		p.drawPixmap(rect(), pix);
	}
}
