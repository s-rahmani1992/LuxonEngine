#include "GraphicWindow.h"

QuantumEngine::Platform::GraphicWindow::GraphicWindow(const WindowProperties& properties, const ATOM& winClass)
    : m_width(properties.width), m_height(properties.height), m_title(properties.title), m_closeFlag(false)
{
    m_handle = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW,
        (LPCWSTR)winClass, m_title.c_str(), WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        50, 50, m_width, m_height, properties.parentWinHandle, nullptr, GetModuleHandleW(nullptr), this);
}

void QuantumEngine::Platform::GraphicWindow::Update(const Float& deltaTime)
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
