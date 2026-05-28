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
		void RenderGraphics(const std::filesystem::path& fileName, ImTextureID texID, bool acceptDrag = false);
		void RenderDirectory(std::filesystem::directory_entry& directory);
		void RenamePath(std::filesystem::path& path, char* newName);
		void DeletePath(std::filesystem::path& path);
		void MoveToFolder(const std::filesystem::path& folder, std::filesystem::path& file);

		std::filesystem::directory_entry m_projectDirectory;
		std::filesystem::directory_entry m_currentDirectory;

		std::vector<std::filesystem::directory_entry> m_folders;
		std::vector<std::filesystem::path> m_folderList;
		std::vector<std::filesystem::path> m_fileList;
		std::function<void()> pendingAction;
		int m_itemWidth = 84;
		int m_itemGap = 20;

		std::filesystem::path* m_selectedItem;
		std::filesystem::path* m_draggedItem;
		bool m_isRenaming = false;
		char m_inputName[256];
		bool m_isDeleting;
		AssetManager* m_assetManager;
	};
}