#include "ConsoleMessageWindow.h"
#include "../Application/EditorApplication.h"
#include "../Application/AssetManager.h"

LuxonEditor::ConsoleMessageWindow::ConsoleMessageWindow()
	:EditorWindow("Console", true), m_assetManager(EditorApplication::GetAssetManager())
{
}

void LuxonEditor::ConsoleMessageWindow::RenderElements()
{
	int messageMask = 0;

	ImGui::BeginGroup();
	ImGui::Checkbox("Logs", &m_logValue);
	messageMask |= (m_logValue ? (int)MessageType::Log : 0);

	ImGui::SameLine(0, 20);

	ImGui::Checkbox("Warnings", &m_warnValue);
	messageMask |= (m_warnValue ? (int)MessageType::Warning : 0);

	ImGui::SameLine(0, 20);

	ImGui::Checkbox("Errors", &m_errorValue);
	messageMask |= (m_errorValue ? (int)MessageType::Error : 0);

	ImGui::SameLine(0, 20);

	if (ImGui::Button("Clear")) {
		MessageLogger::ClearLogs();
	}
	ImGui::EndGroup();

	float availableHeight = ImGui::GetContentRegionAvail().y;

	ImGui::BeginChild("Log Scroll", ImVec2(0, availableHeight), true);

	ImTextureRef texID;
	for (auto& message : MessageLogger::GetMessages()) {
		if ((messageMask & (int)message.type) == 0)
			continue;

		ImGui::BeginGroup();

		switch (message.type) {
		case MessageType::Log:
			texID = m_assetManager->GetIcon(LOG_ICON_ID);
			break;
		case MessageType::Warning:
			texID = m_assetManager->GetIcon(WARN_ICON_ID);
			break;
		case MessageType::Error:
			texID = m_assetManager->GetIcon(ERROR_ICON_ID);
			break;
		}

		ImGui::Image(texID, ImVec2(32, 32));
		ImGui::SameLine(0, 20);
		ImGui::TextWrapped(message.message.c_str());
		ImGui::EndGroup();
	}

	ImGui::EndChild();
}
