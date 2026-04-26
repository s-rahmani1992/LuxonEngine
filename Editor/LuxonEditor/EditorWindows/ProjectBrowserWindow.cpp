
#include "ProjectBrowserWindow.h"
#include "../Application/EditorApplication.h"
#include "../Application/AssetManager.h"


LuxonEditor::ProjectBrowserWindow::ProjectBrowserWindow()
	:EditorWindow("Project Browser"), m_assetManager(EditorApplication::GetAssetManager())
{
}

LuxonEditor::ProjectBrowserWindow::ProjectBrowserWindow(const std::string& projectPath)
	:ProjectBrowserWindow(projectPath, projectPath)
{
}

LuxonEditor::ProjectBrowserWindow::ProjectBrowserWindow(const std::string& projectPath, const std::string& currentPath)
	:EditorWindow("Project Browser"), m_assetManager(EditorApplication::GetAssetManager())
	, m_projectDirectory(std::filesystem::path(projectPath)), m_currentDirectory(std::filesystem::path(currentPath))
{
	UpdatePathList();
}

void LuxonEditor::ProjectBrowserWindow::RenderElements()
{
	if (pendingAction != nullptr) {
		pendingAction();
		pendingAction = nullptr;
	}

	std::filesystem::directory_entry directoryIterator(m_currentDirectory);
	ImGui::BeginGroup();

	RenderDirectory(*(m_folders.rbegin()));
	ImGui::SameLine(0, 10);

	for (auto it = m_folders.rbegin() + 1; it != m_folders.rend(); it++) {
		ImGui::Text(">>");
		ImGui::SameLine(0, 10);
		RenderDirectory(*it);
		ImGui::SameLine(0, 10);
	}

	ImGui::EndGroup();
	

	ImGui::Separator();
	float availableWidth = ImGui::GetContentRegionAvail().x;
	float availableHeight = ImGui::GetContentRegionAvail().y;

	ImGui::BeginChild("Browser Scroll", ImVec2(0, availableHeight), true);

	ImGui::BeginGroup();
	for (auto& folder : m_folderList) {
		RenderFolder(folder);
		
		ImGui::SameLine(0, m_itemGap);
		ImVec2 p = ImGui::GetCursorPos();

		if (p.x + m_itemWidth > availableWidth)
		{
			ImGui::EndGroup();
			ImGui::BeginGroup();
		}
	}

	for (auto& file : m_fileList) {
		RenderFile(file);
		ImGui::SameLine(0, m_itemGap);
		ImVec2 p = ImGui::GetCursorPos();

		if (p.x + m_itemWidth > availableWidth)
		{
			ImGui::EndGroup();
			ImGui::BeginGroup();
		}
	}

	ImGui::EndGroup();

	ImGui::EndChild();
}

void LuxonEditor::ProjectBrowserWindow::UpdatePathList()
{
	m_folders.clear();
	m_folderList.clear();
	m_fileList.clear();

	std::filesystem::directory_entry directoryIterator(m_currentDirectory);

	while (directoryIterator != m_projectDirectory) {
		m_folders.push_back(directoryIterator);
		directoryIterator = std::filesystem::directory_entry(directoryIterator.path().parent_path());
	}

	for (const std::filesystem::directory_entry& childPath : std::filesystem::directory_iterator(m_currentDirectory.path())) {
		if (childPath.is_directory())
			m_folderList.push_back(childPath.path().filename().string());
		else {
			auto extention = childPath.path().extension();
			if (extention.string() == ".json")
				continue;

			m_fileList.push_back(childPath.path());
		}
			
	}
}

void LuxonEditor::ProjectBrowserWindow::OpenFolder(const std::string& folderName)
{
	m_currentDirectory = std::filesystem::directory_entry(m_currentDirectory.path() / folderName);
	UpdatePathList();
}

void LuxonEditor::ProjectBrowserWindow::SetTargetDirectory(std::filesystem::directory_entry& targetDirectory)
{
	m_currentDirectory = targetDirectory;
	UpdatePathList();
}

void LuxonEditor::ProjectBrowserWindow::RenderFolder(const std::string& folderName)
{
	auto texID = m_assetManager->GetIcon(FOLDER_ICON_ID);
	RenderGraphics(folderName, texID);
	
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		pendingAction = [this, folderName]() {
			OpenFolder(folderName);
			};
	}
}

void LuxonEditor::ProjectBrowserWindow::RenderFile(const std::filesystem::path& fileName)
{
	auto texID = m_assetManager->GetIconFromExtention(fileName.extension().string());
	RenderGraphics(fileName.filename().string(), texID);
}

void LuxonEditor::ProjectBrowserWindow::RenderGraphics(const std::string& fileName, ImTextureID texID)
{
	ImVec2 iconSize(m_itemWidth, m_itemWidth);
	float fontSize = ImGui::GetFontSize();
	ImFont* font = ImGui::GetFont();
	float itemWidth = iconSize.x;
	auto requiredTextSize = font->CalcTextSizeA(fontSize, m_itemWidth, m_itemWidth, fileName.c_str());

	float itemHeight = iconSize.y + requiredTextSize.y + m_itemGap; // padding
	ImGui::BeginGroup(); // keep icon + text together

	ImVec2 p = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton(fileName.c_str(), ImVec2(itemWidth, itemHeight));

	ImDrawList* dl = ImGui::GetWindowDrawList();

	if (ImGui::IsItemHovered()) {
		dl->AddRectFilled(ImVec2(p.x - m_itemGap / 2, p.y - m_itemGap / 2), ImVec2(p.x + itemWidth + m_itemGap / 2, p.y + itemHeight + m_itemGap / 2), IM_COL32(0, 0, 255, 100));
	}

	dl->AddImage(texID, p, ImVec2(p.x + iconSize.x, p.y + iconSize.y));

	const char* line_start = fileName.c_str();
	ImVec2 pos = ImVec2(p.x, p.y + m_itemWidth);
	const char* text_end = line_start + fileName.length();
	while (line_start < text_end)
	{
		const char* line_end = font->CalcWordWrapPosition(fontSize, line_start, text_end, m_itemWidth);

		auto textWidth = ImGui::CalcTextSize(line_start, line_end, true, m_itemWidth);

		dl->AddText(ImVec2(pos.x + (m_itemWidth - textWidth.x) / 2, pos.y), IM_COL32_WHITE, line_start, line_end);

		// Advance to next line
		pos.y += fontSize;
		line_start = line_end;

		// Skip spaces/newlines
		while (line_start < text_end && (*line_start == ' ' || *line_start == '\n'))
			line_start++;
	}

	ImGui::EndGroup();
}

void LuxonEditor::ProjectBrowserWindow::RenderDirectory(std::filesystem::directory_entry& directory)
{
	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));

	bool clicked = ImGui::Button(directory.path().filename().string().c_str());

	ImGui::PopStyleColor(3);

	if (clicked) {
		pendingAction = [this, &directory]() {
			SetTargetDirectory(directory);
			};
	}
}
