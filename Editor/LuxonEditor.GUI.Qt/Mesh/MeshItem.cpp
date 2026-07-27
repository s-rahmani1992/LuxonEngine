#include "MeshItem.h"
#include <Core/SerializationStream.h>
#include "MeshViewWindow.h"
#include <QStyleOption>
#include <QPainter>

LuxonEditor::GUI::QT::MeshItem::MeshItem(QWidget* parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setProperty("tag", "MeshItemClass");
	setStyleSheet(styleSheet() + "QWidget[tag='MeshItemClass'] { background-color: #1e1e5e; border-radius: 6px; }");
	char* name = nullptr;
	stream->GetString("name", &name);
	ui.meshName->setText(name);
	ui.infoPanel->layout()->setAlignment(ui.meshName, Qt::AlignLeft);
	ui.infoPanel->layout()->setAlignment(ui.importNameField, Qt::AlignLeft);

	ui.importNameField->InputText()->setText(name);

	connect(ui.viewButton, &QPushButton::clicked, this, [this, stream]() {
		MeshViewWindow* window = new MeshViewWindow(this, stream);
		window->setAttribute(Qt::WA_DeleteOnClose);
		window->setWindowTitle("Mesh Viewer - " + ui.meshName->text());
		window->exec();
		});
}

LuxonEditor::GUI::QT::MeshItem::~MeshItem()
{
}

void LuxonEditor::GUI::QT::MeshItem::paintEvent(QPaintEvent* event)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
	QWidget::paintEvent(event);
}