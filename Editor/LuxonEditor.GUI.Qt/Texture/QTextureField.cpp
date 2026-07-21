#include "QTextureField.h"
#include <qpainter.h>

QTextureField::QTextureField(QWidget *parent, std::string fieldName)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));
	layout()->setAlignment(ui.label, Qt::AlignTop | Qt::AlignLeft);

	ui.textureArea->setFixedSize(90, 90);
	layout()->setAlignment(ui.textureArea, Qt::AlignVCenter | Qt::AlignLeft);

	static_cast<QHBoxLayout*>(layout())->addStretch(1);
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
	update();
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
