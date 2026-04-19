
#include "Application/EditorApplication.h"

// Main code
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    std::string error;
    auto editorApp =  LuxonEditor::EditorApplication::CreateApplication(hInstance, error);
    
    if (editorApp == nullptr)
    {
        return -1;
    }

    editorApp->Run();
    editorApp->ShutDown();

    return 0;
}
