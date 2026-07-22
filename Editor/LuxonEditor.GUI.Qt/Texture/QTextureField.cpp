#include "QTextureField.h"
#include <qpainter.h>
#include <LuxonEditorAPI.h>
#include <qabstractitemview.h>

QTextureField::QTextureField(QWidget *parent, std::string fieldName)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));
	layout()->setAlignment(ui.label, Qt::AlignTop | Qt::AlignLeft);

	ui.textureArea->setFixedSize(90, 90);
	layout()->setAlignment(ui.textureArea, Qt::AlignVCenter | Qt::AlignLeft);
	ui.textureArea->layout()->setAlignment(ui.selectBox, Qt::AlignBottom);
	static_cast<QHBoxLayout*>(layout())->addStretch(1);

	auto textures = GetAssetManager()->GetAllTextureEntries();
	ui.selectBox->view()->setMinimumWidth(200);
	int index = 0;
	int currentIndex;
	for(auto& texture : textures)
	{
		// Do something with each texture entry
		ui.selectBox->addItem(QString::fromStdString(texture->name), QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(texture->guid)));
		ui.selectBox->setItemData(index, QString::fromStdString(texture->name), Qt::ToolTipRole);
		index++;
	}
	
	connect(ui.selectBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
		if (index >= 0)
		{
			auto guid = LuxonEditor::GuidGenerator::GenerateGUIDFromString(ui.selectBox->itemData(index).toString().toStdString());
			auto texture = GetAssetManager()->GetTexture(guid);
			if (texture != m_texture)
			{
				SetTexture(texture);
			}
		}
		});
}

QTextureField::~QTextureField()
{}

void QTextureField::SetTexture(ref<LuxonEngine::Texture2D> texture)
{
	m_textureImage = QImage(
		texture->GetData(),
		texture->GetWidth(),
		texture->GetHeight(),
		texture->GetFormat() == LuxonEngine::TextureFormat::RGBA32 ? QImage::Format_RGBA8888 : QImage::Format_ARGB32
	).copy();

	auto entry = GetAssetManager()->GetTextureEntry(texture);

	int index = ui.selectBox->findData(QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry->guid)));
		if (index >= 0) {
		ui.selectBox->setCurrentIndex(index);
	}

	update();
	emit ValueChanged(texture);
}

void QTextureField::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);
	if (!m_textureImage.isNull())
	{
		QPainter painter(this);
		QRect textureAreaRect = ui.textureArea->geometry();
		QPixmap pixmap = QPixmap::fromImage(m_textureImage);
		painter.drawPixmap(textureAreaRect, pixmap.scaled(textureAreaRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
}
