#pragma once
#include <string>

namespace LuxonEditor {
	class AssetRegistry {
	public:
		AssetRegistry() = default;
		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry(const std::string& projectPath);
		void DeletePath(const std::string& relativePath);

	private:
		std::string m_projectPath;
	};
}