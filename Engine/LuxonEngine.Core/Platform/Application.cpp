#include "Application.h"
#include "GraphicWindow.h"
#include "../Core/Behaviour.h"
#include "../Rendering/GraphicContext.h"

//TODO Move icon to common resource file
#define IDI_QUANTUMENGINETEST           129

QuantumEngine::Platform::Application QuantumEngine::Platform::Application::m_instance = {};

void QuantumEngine::Platform::Application::CreateApplication(HINSTANCE hInstance)
{
    m_instance = Application();
    m_instance.m_app_instance = hInstance;
    m_instance.CreateWindowClass();
}

ref<QuantumEngine::Platform::GraphicWindow> QuantumEngine::Platform::Application::CreateGraphicWindow(const WindowProperties& properties)
{
    m_instance.hostWindow = properties.parentWinHandle;
    EnableWindow(m_instance.hostWindow, FALSE);
	return std::make_shared<GraphicWindow>(properties, m_instance.winClass);
}

std::wstring QuantumEngine::Platform::Application::GetExecutablePath()
{
    LPWSTR rootF = new WCHAR[500];
    DWORD size;
    size = GetModuleFileNameW(NULL, rootF, 500);
    std::wstring root = std::wstring(rootF, size);

    const size_t last_slash_idx = root.rfind('\\');

    if (std::string::npos != last_slash_idx)
        root = root.substr(0, last_slash_idx);

	delete[] rootF;

    return root;
}

void QuantumEngine::Platform::Application::Run(const ref<QuantumEngine::Platform::GraphicWindow>& win, const ref<QuantumEngine::Rendering::GraphicContext>& renderer, const std::vector<ref<Behaviour>>& behaviours)
{
    Int64 countsPerSecond = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSecond);
    double secondsPerCount = 1.0 / (double)countsPerSecond;
    Int32 fps = 60;
    Float targetDelta = 1.0f / fps;
    Float deltaTime = 0.0f;
    Int64 lastCount = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&lastCount);
    Int64 currentCount = 0;
    while (win->ShouldClose() == false) {
        QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);
        win->Update(deltaTime);

        for(const auto& behaviour : behaviours) {
            behaviour->Update(deltaTime);
		}

        renderer->Render();

        deltaTime = secondsPerCount * (currentCount - lastCount);
        lastCount = currentCount;
    }
}

void QuantumEngine::Platform::Application::RunFixed(const ref<GraphicWindow>& win, const ref<Rendering::GraphicContext>& renderer, const std::vector<ref<Behaviour>>& behaviours, UInt32 fps)
{
    Int64 countsPerSecond = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSecond);
    double secondsPerCount = 1.0 / (double)countsPerSecond;
    Float targetDelta = 1.0f / fps;
    Float deltaTime = 0.0f;
    Int64 lastCount = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&lastCount);
    Int64 currentCount = 0;
    while (win->ShouldClose() == false) {
        QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);
        win->Update(deltaTime);

        for (const auto& behaviour : behaviours) {
            behaviour->Update(deltaTime);
        }

        renderer->Render();

        deltaTime = secondsPerCount * (currentCount - lastCount);
        while (deltaTime < targetDelta) {
            QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);
            deltaTime = secondsPerCount * (currentCount - lastCount);
        }
        lastCount = currentCount;
        int fpsd = (int)(1.0f / deltaTime);
    }
}

void QuantumEngine::Platform::Application::Release()
{
    EnableWindow(m_instance.hostWindow, TRUE);
    SetForegroundWindow(m_instance.hostWindow);
    m_instance.m_gpu_device = nullptr;
    UnregisterClass((LPCTSTR)m_instance.winClass, m_instance.m_app_instance);
}

void QuantumEngine::Platform::Application::CreateWindowClass()
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(wcex); //allocation for this window class
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = &OnWindowMessage; // Function pointer for handling messages
    wcex.cbClsExtra = 0; //Allocate additional space for class
    wcex.cbWndExtra = sizeof(GraphicWindow*); //Allocate additional space for each window for storing pointer to created window. Used for getting window in nessage handler
    wcex.hInstance = m_app_instance;
    wcex.hIcon = (HICON)LoadImageW(m_app_instance, MAKEINTRESOURCEW(IDI_QUANTUMENGINETEST), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"WIN_CLASS";
    wcex.hIconSm = (HICON)LoadImageW(m_app_instance, MAKEINTRESOURCEW(IDI_QUANTUMENGINETEST), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_SHARED);
    winClass = RegisterClassExW(&wcex);
}

LRESULT QuantumEngine::Platform::Application::OnWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GraphicWindow* window = reinterpret_cast<GraphicWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE: {
        GraphicWindow* window1 = reinterpret_cast<GraphicWindow*>(((CREATESTRUCTA*)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window1);
        break;
    }
    case WM_DESTROY:
        window->SetCloseFlag(true);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
