#pragma once
#include <Core/EditorWindow.h>
#include <filesystem>
#include <functional>

namespace LuxonEditor {
	class AssetManager;

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
		void RenderFolder(const std::filesystem::path& folderName);
		void RenderFile(const std::filesystem::path& fileName);
		void RenderGraphics(const std::filesystem::path& fileName, ImTextureID texID);
		void RenderDirectory(std::filesystem::directory_entry& directory);
		void RenamePath(std::filesystem::path& path, char* newName);

		std::filesystem::directory_entry m_projectDirectory;
		std::filesystem::directory_entry m_currentDirectory;

		std::vector<std::filesystem::directory_entry> m_folders;
		std::vector<std::filesystem::path> m_folderList;
		std::vector<std::filesystem::path> m_fileList;
		std::function<void()> pendingAction;
		int m_itemWidth = 84;
		int m_itemGap = 20;

		std::filesystem::path* m_selectedItem;
		bool m_isRenaming = false;
		char m_inputName[256];
		AssetManager* m_assetManager;
	};
}