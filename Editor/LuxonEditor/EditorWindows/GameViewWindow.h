#pragma once
#include <Core/EditorWindow.h>

namespace LuxonEditor {
	class GameViewWindow : public EditorWindow {
	public:
		GameViewWindow();

	protected:
		virtual void RenderElements() override;
	};
}