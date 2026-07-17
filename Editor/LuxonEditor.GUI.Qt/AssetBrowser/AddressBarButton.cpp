#include "AddressBarButton.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <Core/Logger.h>
#include <qstyle.h>
#include "AssetBrowserWindow.h"
#include <LuxonEditorAPI.h>
#include <Core/EngineApplication.h>
#include <Core/AssetRegistry.h>

using namespace LuxonEditor::GUI::QT;

AddressBarButton::AddressBarButton(QWidget* parent)
	: QToolButton(parent)
{
	setAcceptDrops(true);
	setAutoRaise(true);
	setCursor(Qt::PointingHandCursor);

	m_originalPalette = palette();
	m_hasOriginalPalette = true;

	setAutoFillBackground(false);
}

void AddressBarButton::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat(DRAG_PATH_MIME_FORMAT) || event->mimeData()->hasUrls())
	{
		event->acceptProposedAction();
		QPalette pal = palette();
		QColor highlight(0, 122, 204, 100);
		pal.setColor(QPalette::Button, highlight);
		setAutoFillBackground(true);
		setPalette(pal);
		update();
	}
	else
	{
		event->ignore();
	}
}

void AddressBarButton::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat(DRAG_PATH_MIME_FORMAT) || event->mimeData()->hasUrls())
	{
		event->acceptProposedAction();
	}
	else
	{
		event->ignore();
	}
}

void AddressBarButton::dragLeaveEvent(QDragLeaveEvent* /*event*/)
{
	if (m_hasOriginalPalette) {
		setPalette(m_originalPalette);
	}
	setAutoFillBackground(false);
	update();
}

void AddressBarButton::dropEvent(QDropEvent* event)
{
	auto mime = event->mimeData();

	if(mime->hasUrls()) {
		auto urls = mime->urls();

		for(auto& url : urls) {
			GetAssetManager()->ImportExternalFile(url.toLocalFile().toStdString(), m_path.toStdString());
		}
	}
	else {
		auto data = mime->data(DRAG_PATH_MIME_FORMAT);
		std::string draggedPath(data.constData(), data.size());

		AssetRegistry_MovePath(GetAssetManager(), draggedPath, m_path.toStdString());
	}

	// reset visual
	if (m_hasOriginalPalette) {
		setPalette(m_originalPalette);
	}
	setAutoFillBackground(false);
	update();
}