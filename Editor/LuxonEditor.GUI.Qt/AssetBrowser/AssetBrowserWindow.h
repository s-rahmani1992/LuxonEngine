#pragma once

#include <QWidget>
#include "ui_AssetBrowserWindow.h"
#include <qfilesystemmodel.h>

#define DRAG_PATH_MIME_FORMAT "path_drag_item"

namespace LuxonEditor {
	class AssetRegistry;
}

namespace LuxonEditor::GUI::QT {
	class PathFilter;
	class PathItemStyle;

	class AssetBrowserWindow : public QWidget
	{
		Q_OBJECT

	public:
		AssetBrowserWindow(QString rootPath, const QString& targetPath, QWidget* parent = nullptr);
		~AssetBrowserWindow();

	protected:
		void keyPressEvent(QKeyEvent* event) override;

	private:
		void RefreshAddressPanel(const QString& targetPath);
		bool eventFilter(QObject* obj, QEvent* event) override;

		Ui::AssetBrowserWindowClass ui;

		QFileSystemModel* m_fileModel;
		PathFilter* m_pathFilter;
		PathItemStyle* m_itemStyle;

		QString m_rootPath;
		LuxonEditor::AssetRegistry* m_assetManager;
		
		// Drag state
		QPoint mousePressPos;
		QModelIndex m_dragStartIndex;
		bool m_isDragActive = false;
		QModelIndex m_lastHoveredIndex;
	};
}
