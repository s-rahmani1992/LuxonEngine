#include "Logger.h"

LuxonEngine::Logger LuxonEngine::Logger::m_instance;

void LuxonEngine::Logger::ClearLogs()
{
	m_instance.m_messages.clear();
	m_instance.InvokeUpdateCallback();
}

size_t LuxonEngine::Logger::RegisterCallback(Callback cb)
{
	auto id = ++m_instance.m_lastCallbackId;
	m_instance.m_callbacks.emplace(id, std::move(cb));
	return id;
}

void LuxonEngine::Logger::UnregisterCallback(size_t id)
{
	m_instance.m_callbacks.erase(id);
}

void LuxonEngine::Logger::LogMessage(const std::string& message, LogType type)
{
	m_messages.push_back(LogData{ .message = message, .type = type });
	InvokeUpdateCallback();
}

void LuxonEngine::Logger::InvokeUpdateCallback()
{
	for (auto& [id, cb] : m_callbacks)
		cb();
}
