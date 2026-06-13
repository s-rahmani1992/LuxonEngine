#pragma once
#include <string>
#include <functional>
#include <memory>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <map>
#include <vector>
#include <chrono>
#include <set>
#include <filesystem>

namespace fs = std::filesystem;

namespace LuxonEditor {
	std::string ToUtf8(const std::wstring& w);

	struct FileChangeEvent {
		std::set<std::string> deletedFiles;
		std::set<std::string> createdFiles;
		std::set<std::string> modifiedFiles;
		std::vector<std::pair<std::string, std::string>> renamedFiles; // old name, new name

		void Initialize(std::map<std::string, int>& rawFiles, fs::path& rootPath);
	};

	class AssetDirectoryWatcher
	{
	public:
		using Callback = std::function<void(const FileChangeEvent& paths)>;

		explicit AssetDirectoryWatcher(const std::string& directory);
		AssetDirectoryWatcher(const AssetDirectoryWatcher&) = delete;
		AssetDirectoryWatcher& operator=(const AssetDirectoryWatcher&) = delete;
		AssetDirectoryWatcher(AssetDirectoryWatcher&&) = default;
		AssetDirectoryWatcher& operator=(AssetDirectoryWatcher&&) = default;

		~AssetDirectoryWatcher();
		bool Start();
		void Stop();
		size_t RegisterCallback(Callback cb);

	private:
		struct OverlappedData {
			OVERLAPPED ov = {};
			BYTE buffer[64 * 1024];
		};

		void WatchLoop();
		void BatchEventLoop();
		void PostRead();
		void ProcessEvents(DWORD bytes);

		std::string m_directory;

		std::thread m_thread;
		std::atomic<bool> m_running;

		HANDLE m_dirHandle = INVALID_HANDLE_VALUE;
		HANDLE m_iocp = nullptr;
		OverlappedData m_ov;

		std::map<size_t, Callback> m_callbacks;
		size_t m_lastCallbackId = 0;

		std::atomic<bool> m_processingEvents{ false };
		std::atomic<std::chrono::steady_clock::time_point> m_lastEventTime;
		std::map<std::string, int> m_fileTracks;
		FileChangeEvent m_currentBatchEvent;
	};
}