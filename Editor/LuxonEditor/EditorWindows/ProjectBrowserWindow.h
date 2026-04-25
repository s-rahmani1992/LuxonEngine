#pragma once
#include <Core/EditorWindow.h>
#include <filesystem>
#include <functional>

namespace LuxonEditor {
	class ProjectBrowserWindow : public EditorWindow {
	public:
		ProjectBrowserWindow();
		ProjectBrowserWindow(const std::string& projectPath);
		ProjectBrowserWindow(const std::string& projectPath, const std::string& currentPath);
	protected:
		virtual void RenderElements() override;

	private:
		void UpdatePathList();
		void OpenFolder(const std::string& folderName);
		void SetTargetDirectory(std::filesystem::directory_entry& targetDirectory);
		void RenderFolder(const std::string& folderName);
		void RenderFile(const std::string& fileName);
		void RenderGraphics(const std::string& fileName, ImU32 color);
		void RenderDirectory(std::filesystem::directory_entry& directory);

		std::filesystem::directory_entry m_projectDirectory;
		std::filesystem::directory_entry m_currentDirectory;

		std::vector<std::filesystem::directory_entry> m_folders;
		std::vector<std::string> m_folderList;
		std::vector<std::string> m_fileList;
		std::function<void()> pendingAction;
		int m_itemWidth = 84;
		int m_itemGap = 20;
	};
}