#pragma once
#include <qstyleditemdelegate.h>
#include <qfilesystemmodel.h>
#include "PathFilter.h"

namespace LuxonEditor::GUI::QT {
    class PathItemStyle : public QStyledItemDelegate {
    public:
        PathItemStyle(PathFilter* proxy, QObject* parent = nullptr);

        void paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override;
        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
            return QSize(80, 140);
        }

    private:
        PathFilter* m_proxy;
        QFileSystemModel* m_fileModel;

        QImage m_folderIcon = QImage(":/LuxonEditorWindow/folder.png");
        QImage m_fileIcon = QImage(":/LuxonEditorWindow/file.png");
    };
}