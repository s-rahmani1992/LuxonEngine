#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define IMGUI_ENABLE_DOCKING

#include <windows.h>
#include <string>

#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>
#include <d3d11.h>
#include <vector>
#include <map>
#include <typeinfo>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace LuxonEditor {
	class EditorWindow;

	class EditorApplication {
	public:
		static EditorApplication* CreateApplication(HINSTANCE hInstance, std::string error);
		EditorApplication(HINSTANCE hInstance);
		bool Initialize(std::string& error);
		void Run();
		void ShutDown();
	private:
		static LRESULT WINAPI OnEditorMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		template<class T, class... Args>
		auto AddWindow(Args&&... args);

		template<typename T>
		const char* GetWindowName();

		bool CreateDeviceD3D();
		void CleanupDeviceD3D();
		void CreateRenderTarget();
		void CleanupRenderTarget();

		static EditorApplication m_appInstance;

		HINSTANCE m_hInstance;
		ATOM m_winClass;
		HWND m_handle;
		int m_width;
		int m_height;
		//ImGuiIO io;

		// DX11 variables
		ID3D11Device* g_pd3dDevice = nullptr;
		ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
		IDXGISwapChain* g_pSwapChain = nullptr;
		ID3D11RenderTargetView* g_mainRenderTargetView = nullptr; 
		bool g_SwapChainOccluded = false;


		// Temporary variables
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		std::vector<EditorWindow*> m_windowList;
		std::map<size_t, EditorWindow*> m_windowMap;
	};

	template<class T, class ...Args>
	inline auto EditorApplication::AddWindow(Args&& ...args)
	{
		EditorWindow* window = new T(std::forward<Args>(args)...);
		m_windowList.push_back(window);
		m_windowMap.emplace(typeid(T).hash_code(), window);
	}

	template<typename T>
	inline const char* EditorApplication::GetWindowName()
	{
		return m_windowMap[typeid(T).hash_code()]->GetWindowName();
	}

}