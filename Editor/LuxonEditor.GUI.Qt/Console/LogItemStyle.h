#pragma once
#include <qstyleditemdelegate.h>


namespace LuxonEditor::GUI::QT {
    class LogItemStyle : public QStyledItemDelegate {
    public:
        void paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override;

        QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
            return QSize(200, 40); // height of each item
        }

    private:
        QImage m_logIcon = QImage(":/LuxonEditorWindow/log.png");
        QImage m_warnIcon = QImage(":/LuxonEditorWindow/warning.png");
        QImage m_errorIcon = QImage(":/LuxonEditorWindow/error.png");
    };
}