#pragma once
#include "gui-pch.h"

namespace LuxonEditor {
	class EditorWindow {
	public:
		EditorWindow(const std::string& windowName, bool isOpen = true);
		void RenderUI();
		const char* GetWindowName() { return m_windowName.c_str(); }
	protected:
		virtual void RenderElements() {}

		std::string m_windowName;
		bool m_isOpen;
	};
}