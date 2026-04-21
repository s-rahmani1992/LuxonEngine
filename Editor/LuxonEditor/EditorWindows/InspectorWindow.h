#pragma once

#pragma once
#include <Core/EditorWindow.h>

namespace LuxonEditor {
	class InspectorWindow : public EditorWindow {
	public:
		InspectorWindow();

	protected:
		virtual void RenderElements() override;
	};
}