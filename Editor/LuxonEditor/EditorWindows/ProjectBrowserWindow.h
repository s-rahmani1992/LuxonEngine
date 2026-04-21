#pragma once
#include <Core/EditorWindow.h>

namespace LuxonEditor {
	class ProjectBrowserWindow : public EditorWindow {
	public:
		ProjectBrowserWindow();

	protected:
		virtual void RenderElements() override;
	};
}