#include "AssetRegistry.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

LuxonEditor::AssetRegistry::AssetRegistry(const std::string& projectPath)
	:m_projectPath(projectPath)
{
}

void LuxonEditor::AssetRegistry::DeletePath(const std::string& relativePath)
{
	fs::path absolutePath = (fs::path(m_projectPath)/ "Assets" / relativePath).lexically_normal();
	if (fs::is_directory(absolutePath))
		fs::remove_all(absolutePath);
	else
		fs::remove(absolutePath);
}

void LuxonEditor::AssetRegistry::RenamePath(const std::string& oldRelativePath, const std::string& newName)
{
	fs::path oldAbsolutePath = (fs::path(m_projectPath) / "Assets" / oldRelativePath).lexically_normal();
	fs::path newAbsolutePath = oldAbsolutePath.parent_path() / (newName + oldAbsolutePath.extension().string());
	fs::path oldMetaPath(oldAbsolutePath.string() + ".json");
	fs::path newMetaPath(newAbsolutePath.string() + ".json");
	std::error_code ec; // to avoid exceptions
	fs::rename(oldAbsolutePath, newAbsolutePath, ec);
	if (ec) {
		return;
	}
	if (fs::exists(oldMetaPath)) {
		fs::rename(oldMetaPath, newMetaPath, ec);
	}
}

void LuxonEditor::AssetRegistry::MovePath(const std::string& oldRelativePath, const std::string& folderRelativePath)
{
	fs::path oldAbsolutePath = (fs::path(m_projectPath) / "Assets" / oldRelativePath).lexically_normal();
	fs::path newAbsolutePath = (fs::path(m_projectPath) / "Assets" / folderRelativePath / oldAbsolutePath.filename()).lexically_normal();
	fs::path oldMetaPath(oldAbsolutePath.string() + ".json");
	fs::path newMetaPath(newAbsolutePath.string() + ".json");
	std::error_code ec; // to avoid exceptions

	if (fs::is_directory(oldAbsolutePath)) {
		fs::create_directory(fs::path(folderRelativePath) / oldAbsolutePath.filename());
		auto subPaths = fs::directory_iterator(oldAbsolutePath);

		for (auto& subPath : subPaths) {
			MovePath(subPath.path().string(), (fs::path(folderRelativePath) / oldAbsolutePath.filename()).string());
		}
		fs::remove(oldAbsolutePath);
		return;
	}

	fs::rename(oldAbsolutePath, newAbsolutePath, ec);
	if (ec) {
		return;
	}
	if (fs::exists(oldMetaPath)) {
		fs::rename(oldMetaPath, newMetaPath, ec);
	}
}
