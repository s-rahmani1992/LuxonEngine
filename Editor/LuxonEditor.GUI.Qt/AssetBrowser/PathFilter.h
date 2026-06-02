#pragma once
#include <qsortfilterproxymodel.h>
#include <Core/Logger.h>

class QFileSystemModel;

namespace LuxonEditor::GUI::QT {
    class PathFilter : public QSortFilterProxyModel {
        Q_OBJECT

    public:
        explicit PathFilter(QFileSystemModel* fileModel, QObject* parent = nullptr);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    private:
		QFileSystemModel* m_fileModel;
    };
}