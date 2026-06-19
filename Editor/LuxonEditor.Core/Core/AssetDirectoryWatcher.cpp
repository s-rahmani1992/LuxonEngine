#include "AssetDirectoryWatcher.h"

#include <stdexcept>

LuxonEditor::AssetDirectoryWatcher::AssetDirectoryWatcher(const std::string& directory)
	: m_directory(directory), m_running(false)
{
}

LuxonEditor::AssetDirectoryWatcher::~AssetDirectoryWatcher()
{
    Stop();
}

bool LuxonEditor::AssetDirectoryWatcher::Start()
{
	if (m_running.load()) return false;
	m_running = true;

    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    m_dirHandle = CreateFileA(
        m_directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (m_dirHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    CreateIoCompletionPort(m_dirHandle, m_iocp, (ULONG_PTR)this, 0);
	m_thread = std::thread(&AssetDirectoryWatcher::WatchLoop, this);
    PostRead();
    return true;
}

void LuxonEditor::AssetDirectoryWatcher::Stop()
{
    if (!m_running.load()) return;
    m_running = false;

    if (m_dirHandle != INVALID_HANDLE_VALUE)
        CancelIo(m_dirHandle);

    if (m_thread.joinable())
        m_thread.join();
}

size_t LuxonEditor::AssetDirectoryWatcher::RegisterCallback(Callback cb)
{
    auto id = ++m_lastCallbackId;
    m_callbacks.emplace(id, std::move(cb));
    return id;
}

void LuxonEditor::AssetDirectoryWatcher::WatchLoop()
{
    while (m_running.load()) {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        OVERLAPPED* ov = nullptr;

        BOOL ok = GetQueuedCompletionStatus(
            m_iocp,
            &bytes,
            &key,
            &ov,
            INFINITE
        );

        if (!m_running.load())
            break;

        if (!ok || ov == nullptr) {
            continue;
        }

        // Process events
        ProcessEvents(bytes);

        // Re-issue async read
        PostRead();
    }

    CloseHandle(m_dirHandle);
    m_dirHandle = INVALID_HANDLE_VALUE;
}

void LuxonEditor::AssetDirectoryWatcher::BatchEventLoop()
{
	m_processingEvents.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto now = std::chrono::steady_clock::now();
	auto lastTimeStamp = m_lastEventTime.load();

    while (now - lastTimeStamp < std::chrono::milliseconds(500)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
		now = std::chrono::steady_clock::now();
    }
    fs::path rootPath = m_directory;
    m_currentBatchEvent.Initialize(m_fileTracks, rootPath);

    // Invoke callbacks
    for (const auto& [id, cb] : m_callbacks) {
        cb(m_currentBatchEvent);
    }

    for (auto& file : m_currentBatchEvent.deletedFiles) {
        fs::remove(fs::path(m_directory) / (file + ".json"));
    }

    m_fileTracks.clear();
	m_processingEvents.store(false);
}

void LuxonEditor::AssetDirectoryWatcher::PostRead()
{
    ZeroMemory(&m_ov.ov, sizeof(OVERLAPPED));

    BOOL ok = ReadDirectoryChangesW(
        m_dirHandle,
        m_ov.buffer,
        sizeof(m_ov.buffer),
        TRUE, // watch subtree
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_SIZE,
        NULL,
        &m_ov.ov,
        NULL
    );
}

void LuxonEditor::AssetDirectoryWatcher::ProcessEvents(DWORD bytes)
{
    BYTE* base = m_ov.buffer;
    FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)base;

    while (true) {
        std::wstring wname(info->FileName, info->FileNameLength / sizeof(WCHAR));
        std::string name = ToUtf8(wname);
        fs::path p = name;

        if (fs::is_directory(p) || !p.has_extension()) {
            if (info->NextEntryOffset == 0)
                break;

            info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);

            continue;
        }

        int inc = 0;

        switch (info->Action) {
        case FILE_ACTION_ADDED:
        case FILE_ACTION_RENAMED_NEW_NAME:
            inc = 1;
            break;

        case FILE_ACTION_RENAMED_OLD_NAME:
        case FILE_ACTION_REMOVED:
            inc = -1;
            break;
        }

        auto nameIT = m_fileTracks.find(name);

        if (nameIT == m_fileTracks.end()) {
            m_fileTracks.emplace(name, inc);
        }
        else {
            (*nameIT).second += inc;
        }

        if (info->NextEntryOffset == 0)
            break;

        info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
    }

	m_lastEventTime.store(std::chrono::steady_clock::now());
    
    if (m_processingEvents.load() == false) {
		m_processingEvents.store(true);
        std::thread(&AssetDirectoryWatcher::BatchEventLoop, this).detach();
    }
}

std::string LuxonEditor::ToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0,
        w.data(), (int)w.size(),
        nullptr, 0,
        nullptr, nullptr
    );

    std::string result(sizeNeeded, 0);

    WideCharToMultiByte(
        CP_UTF8, 0,
        w.data(), (int)w.size(),
        result.data(), sizeNeeded,
        nullptr, nullptr
    );

    return result;
}

void LuxonEditor::FileChangeEvent::Initialize(std::map<std::string, int>& rawFiles, fs::path& rootPath)
{
    deletedFiles.clear();
    createdFiles.clear();
    modifiedFiles.clear();
    renamedFiles.clear();

    while (rawFiles.size() > 0) {
        auto it = rawFiles.begin();
        auto element = std::move(*it);
        rawFiles.erase(it);

        auto path0 = rootPath / element.first;

        if (path0.extension() == ".json") {
            if (element.second == 0) { // meta file is modified
                auto origPath = fs::path(element.first).replace_extension("");

                if (fs::exists(rootPath / origPath)) {
                    modifiedFiles.emplace(origPath.string());

                    auto origElement = rawFiles.find(origPath.string());

                    if (origElement != rawFiles.end())
                        rawFiles.erase(origElement);
                }
            }

            continue;
        }

        if (element.second == 0) {
            auto path = rootPath / element.first;

            if (fs::exists(path)) {
                modifiedFiles.emplace(element.first);

            }
            continue;
        }

        fs::path p1 = element.first;

        auto element2 = std::find_if(rawFiles.begin(), rawFiles.end(), [&p1, &element](auto& name1) {
            fs::path p2 = name1.first;
            bool isFileMoved = (element.second * name1.second < 0) && (p2.filename() == p1.filename());
            bool isFileRenamed = (element.second * name1.second < 0) && (p2.extension() == p1.extension()) && (p1.parent_path() == p2.parent_path());
            return isFileMoved || isFileRenamed;
            });

        if (element2 != rawFiles.end()) {
            if (element.second > 0) {
                renamedFiles.push_back({ (*element2).first, element.first });
            }
            else {
                renamedFiles.push_back({ element.first, (*element2).first });
            }

            rawFiles.erase(element2);
            continue;
        }

        if (element.second > 0) {
            createdFiles.emplace(element.first);
        }
        else {
            deletedFiles.emplace(element.first);
        }
    }
}
