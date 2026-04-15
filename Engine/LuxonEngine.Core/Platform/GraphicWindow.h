#pragma once
#include "../BasicTypes.h"
#include "CommonWin.h"
#include <string>

namespace QuantumEngine::Platform {
	/// <summary>
	/// Properties used for window creation
	/// </summary>
	struct WindowProperties {
	public:
		UInt16 width;
		UInt16 height;
		std::wstring title;
		HWND parentWinHandle = nullptr;
	};

	/// <summary>
	/// Used for handling windows update and render
	/// </summary>
	class GraphicWindow {
	public:
		GraphicWindow(const WindowProperties& properties, const ATOM& winClass);
		inline UInt16 GetWidth() const { return m_width; }
		inline UInt16 GetHeight() const { return m_height; }
		inline HWND GetHandle() const { return m_handle; }
		inline bool ShouldClose() const { return m_closeFlag; }
		void SetCloseFlag(bool close) { m_closeFlag = close; }
		void Update(const Float& deltaTime);
	private:
		HWND m_handle;
		UInt16 m_width;
		UInt16 m_height;
		std::wstring m_title;
		bool m_closeFlag;
	};
}