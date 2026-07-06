#pragma once
#include <string>
#include <functional>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace LuxonEditor {
    class __declspec(dllexport) SelectionManager {
    public:
        using SelectionChangedCallback = std::function<void(const std::string& selectedObject)>;

        SelectionManager() = default;
        SelectionManager(const SelectionManager&) = delete;
        SelectionManager& operator=(const SelectionManager&) = delete;

        const std::string& GetSelectedObject() const;

        void SetSelectedObject(const std::string& selectedObject);

        void SetSelectedObject(const fs::path& selectedPath);

        size_t RegisterSelectionChangedCallback(SelectionChangedCallback callback);

        void UnregisterSelectionChangedCallback(size_t callbackId);

    private:
        std::string m_selectedObject;
        std::vector<std::pair<size_t, SelectionChangedCallback>> m_callbacks;
        size_t m_nextCallbackId = 0;

        // Notify all observers of selection change
        void NotifySelectionChanged();
    };
}
