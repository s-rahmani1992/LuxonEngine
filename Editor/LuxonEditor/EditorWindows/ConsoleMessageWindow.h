#pragma once

#include <Core/EditorWindow.h>
#include "../Application/MessageLogger.h"

namespace LuxonEditor {
	class AssetManager;

	class ConsoleMessageWindow : public EditorWindow {
	public:
		ConsoleMessageWindow();

	protected:
		virtual void RenderElements() override;

	private:
		bool m_logValue = true;
		bool m_warnValue = true;
		bool m_errorValue = true;
		AssetManager* m_assetManager;
	};
}