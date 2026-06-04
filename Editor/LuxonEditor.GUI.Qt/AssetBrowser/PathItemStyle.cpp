#include "PathItemStyle.h"
#include <qfilesystemmodel.h>
#include <qpainter.h>
#include <QLineEdit>
#include <LuxonEditorAPI.h>

LuxonEditor::GUI::QT::PathItemStyle::PathItemStyle(PathFilter* proxy, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_proxy(proxy)
    , m_fileModel(qobject_cast<QFileSystemModel*>(proxy->sourceModel()))
{
}

void LuxonEditor::GUI::QT::PathItemStyle::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    QModelIndex srcIdx = m_proxy->mapToSource(idx);
    p->save();

    QString path = m_fileModel->fileName(srcIdx);
    auto fileInfo = m_fileModel->fileInfo(srcIdx);
	auto iconRect = QRect(opt.rect.left(), opt.rect.top(), opt.rect.width(), opt.rect.width());

    if (opt.state & QStyle::State_Selected) {
        p->fillRect(opt.rect, QColor("#3399ff"));
        p->setPen(Qt::white);
    }
    else if (opt.state & QStyle::State_MouseOver) {
        p->fillRect(opt.rect, QColor("#d0e7ff")); // hover color
    }
    
    if(fileInfo.isDir())
        p->drawImage(iconRect, m_folderIcon);
    else
		p->drawImage(iconRect, m_fileIcon);

    auto textRect = QRect(
        opt.rect.left(),
        iconRect.bottom(),
        opt.rect.width(),
        opt.rect.height() - iconRect.height()
	);

    p->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, fileInfo.completeBaseName());

    p->restore();
	
}

QWidget* LuxonEditor::GUI::QT::PathItemStyle::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QLineEdit* edit = new QLineEdit(parent);
    edit->setFrame(false);
    edit->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    return edit;
}

void LuxonEditor::GUI::QT::PathItemStyle::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QModelIndex srcIdx = m_proxy->mapToSource(index);
    QString name = m_fileModel->fileInfo(srcIdx).completeBaseName();

    QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
    edit->setText(name);
    edit->selectAll();
}

void LuxonEditor::GUI::QT::PathItemStyle::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
    QString newName = edit->text().trimmed();
    if (newName.isEmpty())
        return; // ignore empty rename

    // Get source index (because you use a proxy)
    QModelIndex srcIdx = m_proxy->mapToSource(index);

    QFileInfo info = m_fileModel->fileInfo(srcIdx);
    QString oldPath = info.absoluteFilePath();

	AssetRegistry_RenamePath(GetAssetManager(), oldPath.toStdString(), newName.toStdString());
}

void LuxonEditor::GUI::QT::PathItemStyle::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
    QRect iconRect(opt.rect.left(), opt.rect.top(), opt.rect.width(), opt.rect.width());

    QRect textRect(
        opt.rect.left(),
        iconRect.bottom(),
        opt.rect.width(),
        opt.rect.height() - iconRect.height()
    );

    editor->setGeometry(textRect);
}
