#include "TextureInspecterWidget.h"
#include <Core/SerializationStream.h>
#include <LuxonEditorAPI.h>
#include <QTimer>
#include <QResizeEvent>
#include <QPainter>
#include <QStyleOption>

namespace LuxonEditor::GUI::QT {
	TextureInspecterWidget::TextureInspecterWidget(QWidget* parent, LuxonEngine::SerializationStream* stream, std::string path)
		: QWidget(parent)
	{
		ui.setupUi(this);
		ui.image->installEventFilter(this);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		ui.dimensionLabel->setStyleSheet(ui.dimensionLabel->styleSheet() + "border: 1px solid white; border-radius: 4px; padding: 6px;"); // Optional: Set the text color to white
		ui.dimensionLabel->setWordWrap(true);
		ui.image->setStyleSheet(ui.image->styleSheet() + "background: #222222;"); // Optional: Add a border to the image label
		layout()->setAlignment(ui.dimensionLabel, Qt::AlignTop);

		auto guid = stream->GetGuid("uuid");
		m_texture = GetAssetManager()->GetTexture(guid);

		if(m_texture == nullptr)
		{
			ui.dimensionLabel->setText("Failed to load texture: " + QString::fromStdString(path));
			return;
		}

		QImage::Format format = m_texture->GetFormat() == LuxonEngine::TextureFormat::RGBA32 ? QImage::Format_RGBA8888 : QImage::Format_ARGB32;
		
		m_originalPixmap = QPixmap::fromImage(QImage(
			m_texture->GetData(),
			m_texture->GetWidth(),
			m_texture->GetHeight(),
			format
		).copy());

		ui.dimensionLabel->setText(
			"Dimension: " + QString::number(m_texture->GetWidth()) +
			" x " + QString::number(m_texture->GetHeight()) + "\n" +
			"Format: " + (m_texture->GetFormat() == LuxonEngine::TextureFormat::RGBA32 ? "RGBA32" : "RBGA32")
		);
		update();
	}

	TextureInspecterWidget::~TextureInspecterWidget()
	{
	}

	void TextureInspecterWidget::resizeEvent(QResizeEvent* event)
	{
		update();
		QWidget::resizeEvent(event);
	}

	void TextureInspecterWidget::updateImageDisplay()
	{
		if (m_originalPixmap.isNull())
			return;

		int availableWidth = ui.image->width();
		if (availableWidth <= 0)
			return;

		int scaledHeight = static_cast<int>(
			availableWidth * (float)m_originalPixmap.height() / m_originalPixmap.width()
		);

		ui.image->setFixedHeight(scaledHeight);
		ui.image->setPixmap(m_originalPixmap.scaled(
			availableWidth, scaledHeight,
			Qt::IgnoreAspectRatio,   // already calculated exact size manually
			Qt::SmoothTransformation
		));
	}

	bool TextureInspecterWidget::eventFilter(QObject* watched, QEvent* event)
	{
		if (watched == ui.image && event->type() == QEvent::Paint)
		{
			if(m_texture == nullptr)
				return true;

			QPainter painter(ui.image);

			int availableWidth = ui.image->width();
			int availableHeight = ui.image->height();

			// Draw the stylesheet background first
			QStyleOption opt;
			opt.initFrom(ui.image);
			ui.image->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, ui.image);

			if (m_originalPixmap.isNull())
				return true;

			float textureAspect = (float)m_texture->GetWidth() / m_texture->GetHeight();
			float containerAspect = (float)availableWidth / availableHeight;

			int drawWidth, drawHeight;
			if (textureAspect > containerAspect)
			{
				drawWidth = availableWidth;
				drawHeight = static_cast<int>(availableWidth / textureAspect);
			}
			else
			{
				drawHeight = availableHeight;
				drawWidth = static_cast<int>(availableHeight * textureAspect);
			}

			int x = (availableWidth - drawWidth) / 2;
			int y = (availableHeight - drawHeight) / 2;

			painter.drawPixmap(QRect(x, y, drawWidth, drawHeight), m_originalPixmap);
			return true;
		}
		return QWidget::eventFilter(watched, event);
	}
}
