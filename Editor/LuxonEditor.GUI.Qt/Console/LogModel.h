#pragma once
#include <qwidget.h>
#include <QAbstractListModel>
#include <Qt>

#define LOG_PROPERTIES_MESSAGE_ID 1
#define LOG_PROPERTIES_TYPE_ID 2

namespace LuxonEditor::GUI::QT {
    class LogModel : public QAbstractListModel {
        Q_OBJECT

    public:
        explicit LogModel(QObject* parent = nullptr);

        int rowCount(const QModelIndex&) const override;
        QVariant data(const QModelIndex& index, int role) const override;

        // Call this when Logger changes
        void refresh() {
            beginResetModel();
            endResetModel();
        }
    };

}