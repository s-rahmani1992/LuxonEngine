#include "SelectionManager.h"

namespace LuxonEditor {

    const std::string& SelectionManager::GetSelectedObject() const {
        return m_selectedObject;
    }

    void SelectionManager::SetSelectedObject(const std::string& selectedObject) {
        if (m_selectedObject != selectedObject) {
            m_selectedObject = selectedObject;
            NotifySelectionChanged();
        }
    }

    void SelectionManager::SetSelectedObject(const fs::path& selectedPath) {
        SetSelectedObject(selectedPath.string());
    }

    size_t SelectionManager::RegisterSelectionChangedCallback(SelectionChangedCallback callback) {
        size_t callbackId = m_nextCallbackId++;
        m_callbacks.emplace_back(callbackId, std::move(callback));
        return callbackId;
    }

    void SelectionManager::UnregisterSelectionChangedCallback(size_t callbackId) {
        auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
            [callbackId](const std::pair<size_t, SelectionChangedCallback>& pair) {
                return pair.first == callbackId;
            });
        if (it != m_callbacks.end()) {
            m_callbacks.erase(it);
        }
    }

    void SelectionManager::NotifySelectionChanged() {
        for (auto& pair : m_callbacks) {
            pair.second(m_selectedObject);
        }
    }

}
