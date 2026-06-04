#include "AssetBrowserWindow.h"
#include "PathItemStyle.h"
#include <qtoolbutton.h>
#include <qlabel.h>
#include <Core/Logger.h>
#include <QFileSystemModel>
#include "PathFilter.h"
#include <LuxonEditorAPI.h>
#include <QKeyEvent>
#include <qmessagebox.h>

LuxonEditor::GUI::QT::AssetBrowserWindow::AssetBrowserWindow(QString rootPath, const QString& targetPath, QWidget *parent)
	: m_rootPath(rootPath), m_assetManager(GetAssetManager()), QWidget(parent)
{
	ui.setupUi(this);

	m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(rootPath);
    m_fileModel->setReadOnly(false);
    m_pathFilter = new PathFilter(m_fileModel, this);

	ui.contentListView->setModel(m_pathFilter);
    ui.contentListView->setRootIndex(m_pathFilter->mapFromSource(m_fileModel->index(rootPath)));
	ui.contentListView->setItemDelegate(new PathItemStyle(m_pathFilter));
	ui.contentListView->setViewMode(QListView::IconMode);
	ui.contentListView->setResizeMode(QListView::Adjust);
	ui.contentListView->setMouseTracking(true);
	ui.contentListView->viewport()->setAttribute(Qt::WA_Hover);
	ui.contentListView->installEventFilter(this);
    ui.contentListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

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

void LuxonEditor::GUI::QT::AssetBrowserWindow::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_F2)
    {
        QModelIndex idx = ui.contentListView->currentIndex();
        if (idx.isValid())
            ui.contentListView->edit(idx);
        return;
    }

    QWidget::keyPressEvent(e);
}

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
            RefreshAddressPanel(m_fileModel->filePath(srcIdx));
            });

        layout->addWidget(btn);

        if (i < parts.size() - 1) {
            layout->addWidget(new QLabel("→"));
        }
    }
}

bool LuxonEditor::GUI::QT::AssetBrowserWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui.contentListView && event->type() == QEvent::KeyPress)
    {
        auto* key = (QKeyEvent*)(event);

        if (key->key() == Qt::Key_Delete)
        {
            auto sel = ui.contentListView->selectionModel()->selectedIndexes();

            if (!sel.isEmpty()) {
                auto srcIdx = m_pathFilter->mapToSource(sel.first());
                auto fileInfo = m_fileModel->fileInfo(srcIdx);
				QString message = fileInfo.isDir() ? "Are you sure you want to delete the selected folder and all its contents?" : "Are you sure you want to delete the selected file?";

                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(
                    this,
                    "Confirm Delete",
                    message,
                    QMessageBox::Yes | QMessageBox::No
                );

                if (reply != QMessageBox::Yes) {
					return true; // handled, but do nothing
                }

                for (const auto& idx : sel)
                {
                    auto srcIdx = m_pathFilter->mapToSource(idx);
                    if (!srcIdx.isValid()) continue;
                    auto fileInfo = m_fileModel->fileInfo(srcIdx);
                    QString relativePath = QDir(m_rootPath).relativeFilePath(fileInfo.filePath());

					AssetRegistry_DeletePath(m_assetManager, relativePath.toStdString());
                }

                return true; // handled
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}
