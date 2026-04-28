#pragma once
#include <string>
#include <vector>

namespace LuxonEditor {
	enum class MessageType {
		Log = 1,
		Warning = 1 << 1,
		Error = 1 << 2,
	};

	struct LogMessageData {
		std::string message;
		MessageType type;
	};

	class MessageLogger {
	public:
		inline static void Log(const std::string& message) { m_instance.LogMessage(message, MessageType::Log); }
		inline static void LogWarning(const std::string& message) { m_instance.LogMessage(message, MessageType::Warning); }
		inline static void LogError(const std::string& message) { m_instance.LogMessage(message, MessageType::Error); }
		static void ClearLogs();
		inline static std::vector<LogMessageData>& GetMessages() { return m_instance.m_messages; }
	private:
		void LogMessage(const std::string& message, MessageType type);
		static MessageLogger m_instance;
		std::vector<LogMessageData> m_messages;
	};
}