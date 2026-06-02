#include "PathItemStyle.h"
#include <qfilesystemmodel.h>
#include <qpainter.h>

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

    p->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, path);

    p->restore();
	
}
