#include "MeshItem.h"
#include <Core/SerializationStream.h>
#include "MeshViewWindow.h"

LuxonEditor::GUI::QT::MeshItem::MeshItem(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent)
{
	ui.setupUi(this);
	QPalette pal = this->palette();
	pal.setColor(QPalette::Window, QColor("#1e1e1e"));
	this->setAutoFillBackground(true);
	this->setPalette(pal);
	char* name = nullptr;
	stream->GetString("name", &name);
	ui.meshName->setText(name);
	layout()->setAlignment(ui.meshName, Qt::AlignLeft);

	connect(ui.viewButton, &QPushButton::clicked, this, [this, stream]() {
		MeshViewWindow* window = new MeshViewWindow(this, stream);
		window->setAttribute(Qt::WA_DeleteOnClose);
		window->setWindowTitle("Mesh Viewer - " + ui.meshName->text());
		window->exec();
		});
}

LuxonEditor::GUI::QT::MeshItem::~MeshItem()
{}

