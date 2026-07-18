#include "TextureInspecterWidget.h"
#include <Core/SerializationStream.h>
#include <LuxonEditorAPI.h>
#include <QTimer>
#include <QResizeEvent>

namespace LuxonEditor::GUI::QT {
	TextureInspecterWidget::TextureInspecterWidget(QWidget* parent, LuxonEngine::SerializationStream* stream, std::string path)
		: QWidget(parent)
	{
		ui.setupUi(this);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		auto guid = stream->GetGuid("uuid");
		m_texture = GetAssetManager()->GetTexture(guid);
		QImage::Format format = m_texture->GetFormat() == LuxonEngine::TextureFormat::RGBA32 ? QImage::Format_RGBA8888 : QImage::Format_ARGB32;
		
		m_originalPixmap = QPixmap::fromImage(QImage(
			m_texture->GetData(),
			m_texture->GetWidth(),
			m_texture->GetHeight(),
			format
		).copy());

		ui.image->setAlignment(Qt::AlignTop | Qt::AlignLeft);

		ui.dimensionLabel->setText(
			"Dimension: " + QString::number(m_texture->GetWidth()) +
			" x " + QString::number(m_texture->GetHeight())
		);

		// Initial display once the widget is shown and layout is resolved
		QTimer::singleShot(0, this, [this]() { updateImageDisplay(); });
	}

	TextureInspecterWidget::~TextureInspecterWidget()
	{
	}

	void TextureInspecterWidget::resizeEvent(QResizeEvent* event)
	{
		QWidget::resizeEvent(event);
		// Defer until after the layout engine has distributed the new size
		QTimer::singleShot(0, this, [this]() { updateImageDisplay(); });
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
}
