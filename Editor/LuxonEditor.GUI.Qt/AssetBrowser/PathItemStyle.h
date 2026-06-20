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

		QImage GetIconForIndex(const QModelIndex& idx) const;

    private:
        PathFilter* m_proxy;
        QFileSystemModel* m_fileModel;

        QImage m_folderIcon = QImage(":/LuxonEditorWindow/folder.png");
        QImage m_fileIcon = QImage(":/LuxonEditorWindow/file.png");
        QImage m_hlslIcon = QImage(":/LuxonEditorWindow/hlsl.png");
        QImage m_modelIcon = QImage(":/LuxonEditorWindow/3d-model.png");
        QImage m_textureIcon = QImage(":/LuxonEditorWindow/texture.png");
    };
}