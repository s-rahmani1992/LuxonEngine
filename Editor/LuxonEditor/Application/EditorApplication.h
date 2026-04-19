#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define IMGUI_ENABLE_DOCKING

#include <windows.h>
#include <string>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include <d3d11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace LuxonEditor {
	class EditorApplication {
	public:
		static EditorApplication* CreateApplication(HINSTANCE hInstance, std::string error);
		EditorApplication(HINSTANCE hInstance);
		bool Initialize(std::string& error);
		void Run();
		void ShutDown();
	private:
		static LRESULT WINAPI OnEditorMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	};
}