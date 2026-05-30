#include "LogItemStyle.h"
#include <qpainter.h>
#include "LogModel.h"
#include <Core/Logger.h>

void LuxonEditor::GUI::QT::LogItemStyle::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    p->save();

    p->fillRect(opt.rect, QColor("#ffffff"));

    const int iconSize = 20;
    QRect iconRect = QRect(
        opt.rect.left() + 8,
        opt.rect.top() + (opt.rect.height() - iconSize) / 2,
        iconSize,
        iconSize
    );

    LuxonEngine::LogType logType = (LuxonEngine::LogType)idx.data(LOG_PROPERTIES_TYPE_ID).toInt();

    switch (logType)
    {
    case LuxonEngine::LogType::Log:     p->drawImage(iconRect, m_logIcon); break;
    case LuxonEngine::LogType::Warning: p->drawImage(iconRect, m_warnIcon); break;
    case LuxonEngine::LogType::Error:   p->drawImage(iconRect, m_errorIcon); break;
    }

    QRect textRect = QRect(
        iconRect.right() + 8,
        opt.rect.top(),
        opt.rect.width() - (iconSize + 16),
        opt.rect.height()
    );

    QString text = idx.data(LOG_PROPERTIES_MESSAGE_ID).toString();
    p->drawText(textRect, Qt::AlignVCenter | Qt::TextWordWrap, text);

    p->restore();
}
