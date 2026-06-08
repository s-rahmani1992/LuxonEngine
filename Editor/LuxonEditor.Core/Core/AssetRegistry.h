#pragma once
#include <string>

namespace LuxonEditor {
	class AssetRegistry {
	public:
		AssetRegistry() = default;
		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry(const std::string& projectPath);
		void DeletePath(const std::string& relativePath);
		void RenamePath(const std::string& oldRelativePath, const std::string& newName);
		void MovePath(const std::string& oldRelativePath, const std::string& folderRelativePath);
	private:
		std::string m_projectPath;
	};
}