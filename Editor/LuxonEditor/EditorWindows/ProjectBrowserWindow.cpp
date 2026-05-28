
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

	if (!m_isDeleting && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
		&& ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_selectedItem != nullptr) {
		if (m_isRenaming) {
			m_isRenaming = false;
			RenamePath(*m_selectedItem, m_inputName);
			std::memset(m_inputName, 0, IM_ARRAYSIZE(m_inputName));
		}
		m_selectedItem = nullptr;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_F2) && m_selectedItem != nullptr) {
		m_isRenaming = true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_selectedItem != nullptr) {
		ImGui::OpenPopup("Confirm Delete");
		m_isDeleting = true;
	}

	if (m_isDeleting) {
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
			ImGuiCond_Appearing,
			ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Confirm Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			std::string msg = "Are you sure you want to delete the " + m_selectedItem->filename().string();
			ImGui::Text(msg.c_str());
			ImGui::Separator();

			if (ImGui::Button("Yes", ImVec2(120, 0)))
			{
				DeletePath(*m_selectedItem);     // your action
				m_selectedItem = nullptr;
				m_isDeleting = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine(0, 30);
			if (ImGui::Button("No", ImVec2(120, 0)))
			{
				m_isDeleting = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}

void LuxonEditor::ProjectBrowserWindow::UpdatePathList()
{
	m_folders.clear();
	m_folderList.clear();
	m_fileList.clear();
	m_selectedItem = nullptr;

	std::filesystem::directory_entry directoryIterator(m_currentDirectory);

	while (directoryIterator != m_projectDirectory) {
		m_folders.push_back(directoryIterator);
		directoryIterator = std::filesystem::directory_entry(directoryIterator.path().parent_path());
	}

	for (const std::filesystem::directory_entry& childPath : std::filesystem::directory_iterator(m_currentDirectory.path())) {
		if (childPath.is_directory())
			m_folderList.push_back(childPath.path());
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

void LuxonEditor::ProjectBrowserWindow::RenderFolder(const std::filesystem::path& folderName)
{
	auto texID = m_assetManager->GetIcon(FOLDER_ICON_ID);
	RenderGraphics(folderName, texID);
	
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		pendingAction = [this, folderName]() {
			OpenFolder(folderName.filename().string());
			};
	}
}

void LuxonEditor::ProjectBrowserWindow::RenderFile(const std::filesystem::path& fileName)
{
	auto texID = m_assetManager->GetIconFromExtention(fileName.extension().string());
	RenderGraphics(fileName, texID);
}

void LuxonEditor::ProjectBrowserWindow::RenderGraphics(const std::filesystem::path& fileName, ImTextureID texID, bool acceptDrag)
{
	auto pathStr = fileName.filename().stem().string();
	ImVec2 iconSize(m_itemWidth, m_itemWidth);
	float fontSize = ImGui::GetFontSize();
	ImFont* font = ImGui::GetFont();
	float itemWidth = iconSize.x;
	auto requiredTextSize = font->CalcTextSizeA(fontSize, m_itemWidth, m_itemWidth, pathStr.c_str());

	float itemHeight = iconSize.y + requiredTextSize.y + m_itemGap; // padding
	ImGui::BeginGroup(); // keep icon + text together

	ImVec2 p = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton(pathStr.c_str(), ImVec2(itemWidth, itemHeight));

	ImDrawList* dl = ImGui::GetWindowDrawList();

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
		dl->AddRectFilled(ImVec2(p.x - m_itemGap / 2, p.y - m_itemGap / 2), ImVec2(p.x + itemWidth + m_itemGap / 2, p.y + itemHeight + m_itemGap / 2), IM_COL32(0, 0, 255, 100));
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		m_selectedItem = const_cast<std::filesystem::path*>(&fileName);
	}

	else if(&fileName == m_selectedItem)
		dl->AddRectFilled(ImVec2(p.x - m_itemGap / 2, p.y - m_itemGap / 2), ImVec2(p.x + itemWidth + m_itemGap / 2, p.y + itemHeight + m_itemGap / 2), IM_COL32(0, 0, 255, 200));

	dl->AddImage(texID, p, ImVec2(p.x + iconSize.x, p.y + iconSize.y));
	
	if (m_isRenaming && m_selectedItem == &fileName) {
		auto oldPos = ImGui::GetCursorScreenPos();
		std::memcpy(m_inputName, fileName.filename().stem().string().data(), fileName.filename().string().length());
		ImVec2 pos = ImVec2(p.x, p.y + m_itemWidth);
		ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y));
		ImGui::PushStyleColor(ImGuiCol_InputTextCursor, IM_COL32(0, 0, 0, 255));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(200, 200, 200, 200));        // transparent background
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::SetKeyboardFocusHere();
		ImGui::InputTextMultiline("##edit", m_inputName, IM_ARRAYSIZE(m_inputName), ImVec2(m_itemWidth, fontSize));
	
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(5);

		ImGui::SetCursorScreenPos(oldPos);
		ImGui::Dummy(ImVec2(0, 0));
	}
	else {
		const char* line_start = pathStr.c_str();
		ImVec2 pos = ImVec2(p.x, p.y + m_itemWidth);
		const char* text_end = line_start + pathStr.length();
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
	}

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID |
									ImGuiDragDropFlags_SourceNoPreviewTooltip))
	{
		// Payload: send the file index or pointer
		std::filesystem::path* ptr = const_cast<std::filesystem::path*>(&fileName);
		ImGui::SetDragDropPayload("FILE_ITEM", &ptr, sizeof(std::filesystem::path*));
		
		ImVec2 mouse = ImGui::GetIO().MousePos;
		ImVec2 half = ImVec2(0.5f * iconSize.x, 0.5f * iconSize.y);

		ImDrawList* dl1 = ImGui::GetForegroundDrawList();
		
		// Optional: preview while dragging
		dl1->AddImage(texID, ImVec2(mouse.x - half.x, mouse.y - half.y),
			ImVec2(mouse.x + half.x, mouse.y + half.y), ImVec2(0, 0),
			ImVec2(1, 1), IM_COL32(255, 255, 255, 150));

		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_ITEM")) {
				//auto draggedItem = (std::filesystem::path*)(payload->Data);
				std::filesystem::path* draggedItem = *(std::filesystem::path**)payload->Data;
				if (draggedItem != &fileName && std::filesystem::is_directory(fileName)) {
					pendingAction = [this, fileName, draggedItem]() {
						MoveToFolder(fileName, *draggedItem);
						};
				}
			}
		}
		
		ImGui::EndDragDropTarget();
	}

	ImGui::EndGroup();
}

void LuxonEditor::ProjectBrowserWindow::RenderDirectory(std::filesystem::directory_entry& directory)
{
	ImGui::BeginGroup();
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

	if (ImGui::BeginDragDropTarget())
	{
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_ITEM")) {
				std::filesystem::path* draggedItem = *(std::filesystem::path**)payload->Data;
				auto& draggedPath = *draggedItem;
				if (draggedPath.parent_path() != directory.path()) {
					pendingAction = [this, directory, &draggedPath]() {
						MoveToFolder(directory.path(), draggedPath);
						};
				}
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::EndGroup();
}

void LuxonEditor::ProjectBrowserWindow::RenamePath(std::filesystem::path& path, char* newName)
{
	std::filesystem::path newPath = (path.parent_path() / (newName + path.extension().string()));
	std::filesystem::path metaPath(path.string() + ".json");
	std::filesystem::path newMetaPath = (metaPath.parent_path() / (newName + path.extension().string() + ".json"));
	std::filesystem::rename(path, newPath);
	if(std::filesystem::exists(metaPath))
		std::filesystem::rename(metaPath, newMetaPath);
	UpdatePathList();
}

void LuxonEditor::ProjectBrowserWindow::DeletePath(std::filesystem::path& path)
{
	std::filesystem::path metaPath(path.string() + ".json");
	
	if (std::filesystem::is_directory(path))
		std::filesystem::remove_all(path);
	else
		std::filesystem::remove(path);
	
	std::filesystem::remove(metaPath);
	UpdatePathList();
}

void LuxonEditor::ProjectBrowserWindow::MoveToFolder(const std::filesystem::path& folder, std::filesystem::path& file)
{
	std::filesystem::path newPath = (folder / (file.filename()));
	std::filesystem::path metaPath(file.string() + ".json");

	if (!std::filesystem::is_directory(file) && std::filesystem::exists(metaPath)) {
		
		std::filesystem::path newMetaPath(newPath.string() + ".json");
		std::filesystem::rename(metaPath, newMetaPath);
	}

	std::filesystem::rename(file, newPath);
	UpdatePathList();
}
