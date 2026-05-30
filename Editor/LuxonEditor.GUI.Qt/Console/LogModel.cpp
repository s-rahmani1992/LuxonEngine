#include "LogModel.h"
#include <EngineAPI.h>

using namespace LuxonEngine;

LuxonEditor::GUI::QT::LogModel::LogModel(QObject* parent)
    : QAbstractListModel(parent) {
    Logger::RegisterCallback([this]() {
        refresh();
        });
}

int LuxonEditor::GUI::QT::LogModel::rowCount(const QModelIndex&) const
{
    return Logger::GetMessages().size();
}

QVariant LuxonEditor::GUI::QT::LogModel::data(const QModelIndex& index, int role) const
{
    const auto& logs = Logger::GetMessages();
    const LogData& entry = logs[index.row()];

    switch (role) {
        case LOG_PROPERTIES_MESSAGE_ID:
            return QString::fromStdString(entry.message);
        case LOG_PROPERTIES_TYPE_ID:
            return static_cast<int>(entry.type);
    }

    return {};
}
