#include "MessageLogger.h"

LuxonEditor::MessageLogger LuxonEditor::MessageLogger::m_instance;

void LuxonEditor::MessageLogger::ClearLogs()
{
	m_instance.m_messages.clear();
}

void LuxonEditor::MessageLogger::LogMessage(const std::string& message, MessageType type)
{
	m_messages.push_back(LogMessageData{ .message = message, .type = type });
}
