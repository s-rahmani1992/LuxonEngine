#include "GameViewWindow.h"

LuxonEditor::GameViewWindow::GameViewWindow()
	:EditorWindow("Game", true)
{
}

void LuxonEditor::GameViewWindow::RenderElements()
{
	ImGui::Text("Game View goes here");
}
