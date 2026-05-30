#pragma once
#include <qsortfilterproxymodel.h>
#include <Core/Logger.h>

namespace LuxonEditor::GUI::QT {
    class LogFilter : public QSortFilterProxyModel {
        Q_OBJECT

    public:
        explicit LogFilter(QObject* parent = nullptr)
            : QSortFilterProxyModel(parent) {
        }

        void SetMask(LuxonEngine::LogType logType, bool isOn);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    private:
        int allowedMask = 0;
    };
}