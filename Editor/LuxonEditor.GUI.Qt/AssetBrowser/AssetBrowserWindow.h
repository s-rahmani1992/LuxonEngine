#pragma once

#include <QWidget>
#include "ui_AssetBrowserWindow.h"
#include <qfilesystemmodel.h>

namespace LuxonEditor {
	class AssetRegistry;
}

namespace LuxonEditor::GUI::QT {
	class PathFilter;

	class AssetBrowserWindow : public QWidget
	{
		Q_OBJECT

	public:
		AssetBrowserWindow(QString rootPath, const QString& targetPath, QWidget* parent = nullptr);
		~AssetBrowserWindow();

	private:
		void RefreshAddressPanel(const QString& targetPath);
		bool eventFilter(QObject* obj, QEvent* event) override;

		Ui::AssetBrowserWindowClass ui;

		QFileSystemModel* m_fileModel;
		PathFilter* m_pathFilter;

		QString m_rootPath;
		LuxonEditor::AssetRegistry* m_assetManager;
	};
}
