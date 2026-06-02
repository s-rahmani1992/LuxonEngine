#include "AssetBrowserWindow.h"
#include "PathItemStyle.h"
#include <qtoolbutton.h>
#include <qlabel.h>
#include <Core/Logger.h>
#include <QFileSystemModel>
#include "PathFilter.h"

LuxonEditor::GUI::QT::AssetBrowserWindow::AssetBrowserWindow(QString rootPath, const QString& targetPath, QWidget *parent)
	: m_rootPath(rootPath), QWidget(parent)
{
	ui.setupUi(this);

	m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(rootPath);
    m_pathFilter = new PathFilter(m_fileModel, this);

	ui.contentListView->setModel(m_pathFilter);
    ui.contentListView->setRootIndex(m_pathFilter->mapFromSource(m_fileModel->index(rootPath)));
	ui.contentListView->setItemDelegate(new PathItemStyle(m_pathFilter));
	ui.contentListView->setViewMode(QListView::IconMode);
	ui.contentListView->setResizeMode(QListView::Adjust);
	ui.contentListView->setMouseTracking(true);
	ui.contentListView->viewport()->setAttribute(Qt::WA_Hover);

	RefreshAddressPanel(targetPath);

	connect(ui.contentListView, &QListView::doubleClicked, this, [this](const QModelIndex& idx) {
        QModelIndex srcIdx = m_pathFilter->mapToSource(idx);
        if (!srcIdx.isValid()) return;

        auto fileInfo = m_fileModel->fileInfo(srcIdx);
        if (fileInfo.isDir())
        {
            ui.contentListView->setRootIndex(idx);
            ui.contentListView->clearSelection();
            RefreshAddressPanel(m_fileModel->filePath(srcIdx));
        }
	});
}

LuxonEditor::GUI::QT::AssetBrowserWindow::~AssetBrowserWindow()
{}

void LuxonEditor::GUI::QT::AssetBrowserWindow::RefreshAddressPanel(const QString& targetPath)
{
    auto* layout = ui.addressPanel->layout();

    // Clear previous items
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QString root = QDir::cleanPath(m_rootPath);
    QString target = QDir::cleanPath(targetPath);
	QString parentPath = QFileInfo(root).dir().path();
    QString relative = QDir(parentPath).relativeFilePath(target);
    QStringList parts = relative.split("/", Qt::SkipEmptyParts);
    QString currentPath = parentPath;

    for (int i = 0; i < parts.size(); i++)
    {
        currentPath += "/" + parts[i];

        QToolButton* btn = new QToolButton(this);
        btn->setText(parts[i]);
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);

        QString pathCopy = currentPath;
        connect(btn, &QToolButton::clicked, this, [this, pathCopy]() {
            m_fileModel->setRootPath(pathCopy);
            QModelIndex srcIdx = m_fileModel->index(pathCopy);
            ui.contentListView->setRootIndex(m_pathFilter->mapFromSource(srcIdx));
            ui.contentListView->clearSelection();
            });

        layout->addWidget(btn);

        if (i < parts.size() - 1) {
            layout->addWidget(new QLabel("→"));
        }
    }
}

