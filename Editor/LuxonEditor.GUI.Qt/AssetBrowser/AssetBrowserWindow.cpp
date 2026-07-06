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
#include <qmimedata.h>
#include <QDrag>
#include "AddressBarButton.h"
#include <QDesktopServices>
#include <QUrl>

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
	m_itemStyle = new PathItemStyle(m_pathFilter);
	ui.contentListView->setItemDelegate(m_itemStyle);
	ui.contentListView->setViewMode(QListView::IconMode);
	ui.contentListView->setResizeMode(QListView::Adjust);
	ui.contentListView->setMouseTracking(true);
	ui.contentListView->viewport()->setAttribute(Qt::WA_Hover);
	ui.contentListView->installEventFilter(this);
    ui.contentListView->viewport()->installEventFilter(this);
    ui.contentListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Keep automatic drag start disabled; we start drags manually.
    ui.contentListView->setDragEnabled(false);

    // Ensure the view and viewport accept drops so drag events are delivered.
    ui.contentListView->setAcceptDrops(true);
    ui.contentListView->viewport()->setAcceptDrops(true);
    ui.contentListView->viewport()->setAttribute(Qt::WA_AcceptDrops);

    // Allow the view to receive drag/drop events (we will intercept them and prevent internal moves).
    ui.contentListView->setDropIndicatorShown(false);
    ui.contentListView->setDragDropMode(QAbstractItemView::DragDrop);

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
        else
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.filePath()));
        }
	});

    connect(ui.contentListView, &QListView::clicked, this, [this](const QModelIndex& idx) {
        QModelIndex srcIdx = m_pathFilter->mapToSource(idx);
        if (!srcIdx.isValid()) return;

        auto fileInfo = m_fileModel->fileInfo(srcIdx);
		GetSelectionManager()->SetSelectedObject(fileInfo.filePath().toStdString());
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

        AddressBarButton* btn = new AddressBarButton(this);
        btn->setText(parts[i]);
        btn->setPath(currentPath);
        btn->setStyleSheet(""); // optional: ensure default appearance

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
    // Handle key delete on the view itself
    if (obj == ui.contentListView && event->type() == QEvent::KeyPress)
    {
        auto* key = static_cast<QKeyEvent*>(event);

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

    // Handle mouse/drag events on the viewport (and also handle drag events if delivered to the view)
    if ((obj == ui.contentListView->viewport() || obj == ui.contentListView))
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        {
            auto* me = static_cast<QMouseEvent*>(event);
            mousePressPos = me->pos();
            // Save index relative to viewport if needed
            m_dragStartIndex = ui.contentListView->indexAt(mousePressPos);
            return false;
        }

        case QEvent::MouseMove:
        {
            auto* me = static_cast<QMouseEvent*>(event);

            if ((me->buttons() & Qt::LeftButton) && m_dragStartIndex.isValid() && !m_isDragActive)
            {
                if ((me->pos() - mousePressPos).manhattanLength() >= QApplication::startDragDistance())
                {
                    // Begin manual drag
                    m_isDragActive = true;

                    QModelIndex srcIdx = m_pathFilter->mapToSource(m_dragStartIndex);
                    if (srcIdx.isValid())
                    {
                        QString path = m_fileModel->filePath(srcIdx);

                        QMimeData* mime = new QMimeData();
                        mime->setData(DRAG_PATH_MIME_FORMAT, path.toUtf8());

                        // Use the viewport as parent for the drag (source widget is this widget).
                        QDrag drag(ui.contentListView);
                        drag.setMimeData(mime);
                        
                        // --- PREVIEW ICON: obtain model's icon and set as pixmap ---
                        QImage icon = m_itemStyle->GetIconForIndex(srcIdx);
                        QPixmap pix;
                        // Choose a preview size (tweak if needed to match your delegate)
                        int previewSize = m_itemStyle->sizeHint(QStyleOptionViewItem(), srcIdx).width();
                        if (!icon.isNull()) {
                            pix = QPixmap::fromImage(icon).scaled(previewSize, previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        }
                        if (pix.isNull()) {
                            // fallback: tiny transparent pixmap to avoid platform quirks
                            pix = QPixmap(previewSize, previewSize);
                            pix.fill(Qt::transparent);
                        }
                        // Optional: add a small drop shadow or scaling if desired
                        drag.setPixmap(pix);
                        // Put hot spot near cursor center so it feels natural
                        drag.setHotSpot(QPoint(pix.width() / 2, pix.height() / 2));
                        
                        // --- end preview setup ---
                        Qt::DropAction result = drag.exec(Qt::CopyAction | Qt::MoveAction);
                        Q_UNUSED(result);
                    }

                    m_isDragActive = false;
                    m_dragStartIndex = QModelIndex();
                    return true; // we started and handled the drag
                }
            }

            return false;
        }

        case QEvent::MouseButtonRelease:
        {
            m_isDragActive = false;
            m_dragStartIndex = QModelIndex();
            return false;
        }

        case QEvent::DragEnter: {
            auto* de = static_cast<QDragEnterEvent*>(event);
            de->acceptProposedAction();
            return true; // handled
        }
        
        case QEvent::DragMove:
        {
            auto* de = static_cast<QDropEvent*>(event);

            if(de->mimeData()->hasFormat(DRAG_PATH_MIME_FORMAT) == false) {
                de->ignore();
                return true;
			}

            QModelIndex index = ui.contentListView->indexAt(static_cast<QDragMoveEvent*>(event)->position().toPoint());
			
            if (index.isValid() == false) {
                de->ignore();
            }
            else {
                if (index == m_dragStartIndex) {
                    de->ignore();
                }
                else {
					auto pathInfo = m_fileModel->fileInfo(m_pathFilter->mapToSource(index));
                    if(pathInfo.isDir()) {
                        de->acceptProposedAction();
                    }
                    else {
                        de->ignore();
					}
				}
            }

            m_lastHoveredIndex = index;
            return true;
        }

        case QEvent::DragLeave:
            return true;

        case QEvent::Drop: {
            auto* de = static_cast<QDropEvent*>(event);

			auto mime = de->mimeData();
            if (mime->hasFormat(DRAG_PATH_MIME_FORMAT))
            {
                QByteArray data = mime->data(DRAG_PATH_MIME_FORMAT);
                std::string draggedPath = std::string(data.constData(), data.size());

                if (!m_lastHoveredIndex.isValid()) {
                    de->ignore();
                    return true;
                }
                auto targetInfo = m_fileModel->fileInfo(m_pathFilter->mapToSource(m_lastHoveredIndex));
                if (!targetInfo.isDir()) {
                    de->ignore();
                    return true;
                }

                QString targetPath = targetInfo.filePath();
				AssetRegistry_MovePath(GetAssetManager(), draggedPath, targetPath.toStdString());
                return true;
            }
        }

        default:
            break;
        }
    }

    return QWidget::eventFilter(obj, event);
}
