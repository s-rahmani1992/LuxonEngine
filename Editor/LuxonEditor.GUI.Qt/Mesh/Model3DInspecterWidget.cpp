#include "Model3DInspecterWidget.h"
#include <Core/SerializationStream.h>
#include "MeshItem.h"

LuxonEditor::GUI::QT::Model3DInspecterWidget::Model3DInspecterWidget(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent)
{
	ui.setupUi(this);

	QPalette pal = this->palette();
	pal.setColor(QPalette::Window, QColor("#8e1e1e"));
	this->setAutoFillBackground(true);
	this->setPalette(pal);

	ui.meshList->setStyleSheet("#meshList { background-color: #2b2b5b; }");

	std::vector<LuxonEngine::SerializationStream> meshes = stream->Array("meshes");

	for (auto& meshStream : meshes)
	{
		MeshItem* item = new MeshItem(ui.meshList, &meshStream);
		ui.meshList->layout()->addWidget(item);
		ui.meshList->layout()->setAlignment(item, Qt::AlignTop);
	}
}

LuxonEditor::GUI::QT::Model3DInspecterWidget::~Model3DInspecterWidget()
{}

