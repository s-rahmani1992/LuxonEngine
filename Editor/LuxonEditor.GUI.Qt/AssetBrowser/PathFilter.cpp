#include "PathFilter.h"
#include <qfilesystemmodel.h>

LuxonEditor::GUI::QT::PathFilter::PathFilter(QFileSystemModel* fileModel, QObject* parent)
    : QSortFilterProxyModel(parent), m_fileModel(fileModel) {
    setSourceModel(fileModel);
}

bool LuxonEditor::GUI::QT::PathFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex srcIndex = m_fileModel->index(sourceRow, 0, sourceParent);
    QString path = sourceModel()->data(srcIndex, QFileSystemModel::FilePathRole).toString();

    if (path.endsWith(".json", Qt::CaseInsensitive))
        return false;

    return true;
}
