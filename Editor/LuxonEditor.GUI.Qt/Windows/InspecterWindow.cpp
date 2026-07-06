#include "InspecterWindow.h"
#include <LuxonEditorAPI.h>

LuxonEditor::GUI::QT::InspecterWindow::InspecterWindow(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	GetSelectionManager()->RegisterSelectionChangedCallback([this](const std::string& selectedObject) {
		ui.label->setText(QString::fromStdString(selectedObject));
		});
}

LuxonEditor::GUI::QT::InspecterWindow::~InspecterWindow()
{}

