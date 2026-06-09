#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "../export.h"

namespace LuxonEngine {
	enum class LogType {
		Log = 1,
		Warning = 1 << 1,
		Error = 1 << 2,
	};

	struct LogData {
		std::string message;
		LogType type;
	};

	class LUXON_CORE_API Logger {
	public:
		using Callback = std::function<void()>;
		inline static void Log(const std::string& message) { m_instance.LogMessage(message, LogType::Log); }
		inline static void LogWarning(const std::string& message) { m_instance.LogMessage(message, LogType::Warning); }
		inline static void LogError(const std::string& message) { m_instance.LogMessage(message, LogType::Error); }
		static void ClearLogs();
		inline static std::vector<LogData>& GetMessages() { return m_instance.m_messages; }
		static size_t RegisterCallback(Callback cb);

		static void UnregisterCallback(size_t id);
	private:
		void LogMessage(const std::string& message, LogType type);
		void InvokeUpdateCallback();
		static Logger m_instance;
		std::vector<LogData> m_messages;
		std::map<size_t, Callback> m_callbacks;
		size_t m_lastCallbackId = 0;
	};
}