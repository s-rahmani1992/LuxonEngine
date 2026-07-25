#include "LogItemStyle.h"
#include <qpainter.h>
#include "LogModel.h"
#include <Core/Logger.h>

void LuxonEditor::GUI::QT::LogItemStyle::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    p->save();

    p->fillRect(opt.rect, QColor("#111111"));

    p->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());

    const int iconSize = 20;
    QRect iconRect = QRect(
        opt.rect.left() + 8,
        opt.rect.top() + 8,
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
    p->drawText(textRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWrapAnywhere, text);
	
    p->restore();
}

QSize LuxonEditor::GUI::QT::LogItemStyle::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    const int iconSize = 20;
    const int padding = 16;
    const int minHeight = iconSize + 8;

    QString text = idx.data(LOG_PROPERTIES_MESSAGE_ID).toString();

    QRect textRect(0, 0, opt.rect.width() - (iconSize + padding), 10000);
    QFontMetrics fm(opt.font);
    QRect boundingRect = fm.boundingRect(textRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWrapAnywhere, text);

    int height = qMax(minHeight, boundingRect.height() + 8);

    return QSize(opt.rect.width(), height);
}
