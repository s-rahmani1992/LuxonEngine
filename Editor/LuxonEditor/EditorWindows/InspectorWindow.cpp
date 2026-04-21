#include "InspectorWindow.h"

LuxonEditor::InspectorWindow::InspectorWindow()
	:EditorWindow("Inspector", true)
{
}

void LuxonEditor::InspectorWindow::RenderElements()
{
	ImGui::Text("Inspector goes here");
}
