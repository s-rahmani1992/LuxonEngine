#include "MeshItem.h"
#include <Core/SerializationStream.h>

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
}

LuxonEditor::GUI::QT::MeshItem::~MeshItem()
{}

