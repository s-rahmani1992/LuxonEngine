#include "gui-pch.h"
#include "EditorWindow.h"

LuxonEditor::EditorWindow::EditorWindow(const std::string& windowName, bool isOpen)
    :m_windowName(windowName), m_isOpen(isOpen)
{

}

void LuxonEditor::EditorWindow::RenderUI()
{
    if (!m_isOpen)
        return;

    if (ImGui::Begin(m_windowName.c_str(), &m_isOpen)) {
        RenderElements();
    }
    
    ImGui::End();
}
