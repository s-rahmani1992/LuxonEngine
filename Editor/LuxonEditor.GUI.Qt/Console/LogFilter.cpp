#include "LogFilter.h"
#include <Core/Logger.h>
#include "LogModel.h"

void LuxonEditor::GUI::QT::LogFilter::SetMask(LuxonEngine::LogType logType, bool isOn)
{
    if (isOn)
        allowedMask |= ((int)logType);
    else
        allowedMask &= ~((int)logType);

    invalidateFilter();
}

bool LuxonEditor::GUI::QT::LogFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    return (idx.data(LOG_PROPERTIES_TYPE_ID).toInt() & allowedMask) != 0;
}
