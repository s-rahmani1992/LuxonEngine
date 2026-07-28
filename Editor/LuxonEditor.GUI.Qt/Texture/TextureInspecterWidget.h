#pragma once

#include <QWidget>
#include <QPixmap>
#include "ui_TextureInspecterWidget.h"
#include <EngineAPI.h>

namespace LuxonEngine {
	class SerializationStream;
	class Texture2D;
}

namespace LuxonEditor::GUI::QT {
	class TextureInspecterWidget : public QWidget
	{
		Q_OBJECT

	public:
		TextureInspecterWidget(QWidget* parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr, std::string path = "");
		~TextureInspecterWidget();
	protected:
		virtual void resizeEvent(QResizeEvent* event) override;
		bool eventFilter(QObject* watched, QEvent* event) override;
	private:
		void updateImageDisplay();
		Ui::TextureInspecterWidgetClass ui;
		ref<LuxonEngine::Texture2D> m_texture;
		QPixmap m_originalPixmap;
	};
}
