#pragma once

#include <QDialog>
#include "ui_LuxonStartWindow.h"
#include "EngineAPI.h"

namespace LuxonEditor::GUI::QT {
	class LuxonStartWindow : public QDialog
	{
		Q_OBJECT

	public:
		LuxonStartWindow(QWidget* parent = nullptr);
		~LuxonStartWindow();

		Graphic_API GetSelectedGraphicAPI() const { return m_graphicAPI; }

	protected:
		virtual void paintEvent(QPaintEvent*) override;

	private:
		Ui::LuxonStartWindowClass ui;

		Graphic_API m_graphicAPI;
		QPixmap pix;
	};
}
