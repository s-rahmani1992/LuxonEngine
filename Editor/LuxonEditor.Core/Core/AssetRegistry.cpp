#include "AssetRegistry.h"
#include <filesystem>

namespace fs = std::filesystem;

LuxonEditor::AssetRegistry::AssetRegistry(const std::string& projectPath)
	:m_projectPath(projectPath)
{
}

void LuxonEditor::AssetRegistry::DeletePath(const std::string& relativePath)
{
	fs::path absolutePath = (fs::path(m_projectPath)/ "Assets" / relativePath).lexically_normal();
	fs::path metaPath(absolutePath.string() + ".json");
	if (fs::is_directory(absolutePath))
		fs::remove_all(absolutePath);
	else
		fs::remove(absolutePath);

	fs::remove(metaPath);
}