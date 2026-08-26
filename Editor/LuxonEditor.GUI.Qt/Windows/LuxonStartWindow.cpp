#include "LuxonStartWindow.h"
#include <QPainter>

namespace LuxonEditor::GUI::QT {
	LuxonStartWindow::LuxonStartWindow(QWidget* parent)
		: QDialog(parent), m_graphicAPI(Graphic_API::DIRECTX_12),
		  pix(":/LuxonEditorWindow/splash.png")
	{
		ui.setupUi(this);
		setFixedSize(450, 320);

		ui.uiPanel->layout()->setAlignment(Qt::AlignCenter);
		ui.buttonPanel->layout()->setAlignment(Qt::AlignCenter);

		static_cast<QHBoxLayout*>(ui.buttonPanel->layout())->insertStretch(0, 1);

		connect(ui.closeButton, &QPushButton::clicked, this, &LuxonStartWindow::reject);

		connect(ui.openButton, &QPushButton::clicked, this, &LuxonStartWindow::accept);

		connect(ui.apiBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
			switch (index)
			{
			case 0:
				m_graphicAPI = Graphic_API::DIRECTX_12;
				break;
			case 1:
				m_graphicAPI = Graphic_API::VULKAN;
				break;
			default:
				m_graphicAPI = Graphic_API::DIRECTX_12;
				break;
			}
			});
	}

	LuxonStartWindow::~LuxonStartWindow()
	{
	}
	void LuxonStartWindow::paintEvent(QPaintEvent*)
	{
		QPainter p(this);
		p.drawPixmap(ui.image->rect(), pix);

	}
}
