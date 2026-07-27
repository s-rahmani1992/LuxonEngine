#include "MeshItem.h"
#include <Core/SerializationStream.h>
#include "MeshViewWindow.h"
#include <QStyleOption>
#include <QPainter>

LuxonEditor::GUI::QT::MeshItem::MeshItem(QWidget* parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent), m_originalStream(*stream), m_currentStream(*stream)
{
	ui.setupUi(this);
	this->setProperty("tag", "MeshItemClass");
	setStyleSheet(styleSheet() + "QWidget[tag='MeshItemClass'] { background-color: #1e1e5e; border-radius: 6px; }");
	char* name = nullptr;
	stream->GetString("name", &name);
	ui.meshName->setText(name);
	ui.infoPanel->layout()->setAlignment(ui.meshName, Qt::AlignLeft);
	ui.infoPanel->layout()->setAlignment(ui.importNameField, Qt::AlignLeft);

	char* importedName = nullptr;
	if (stream->GetString("imported_name", &importedName) == false)
		importedName = name;
	ui.importNameField->InputText()->setText(importedName);

	connect(ui.viewButton, &QPushButton::clicked, this, [this, stream]() {
		MeshViewWindow* window = new MeshViewWindow(this, stream);
		window->setAttribute(Qt::WA_DeleteOnClose);
		window->setWindowTitle("Mesh Viewer - " + ui.meshName->text());
		window->exec();
		});

	connect(ui.importNameField, &QTextField::ValueChanged, this, [this](bool isValid) {
		if (isValid)
		{
			QString newName = ui.importNameField->InputText()->text();
			m_currentStream.SetString("imported_name", newName.toStdString());
		}
		else
		{
			m_currentStream.SetString("imported_name", ui.meshName->text().toStdString());
		}
		});
}

LuxonEditor::GUI::QT::MeshItem::~MeshItem()
{
}

void LuxonEditor::GUI::QT::MeshItem::Revert()
{
	char* originalName;
	if(m_originalStream.GetString("imported_name", &originalName) == false)
		m_originalStream.GetString("name", &originalName);
	ui.importNameField->InputText()->setText(originalName);
}

void LuxonEditor::GUI::QT::MeshItem::paintEvent(QPaintEvent* event)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
	QWidget::paintEvent(event);
}