#pragma once
#include <string>
#include <map>
#include <EngineAPI.h>
#include <boost/uuid/uuid.hpp>
#include <filesystem>

namespace fs = std::filesystem;

namespace LuxonEditor {
	class AssetDirectoryWatcher;

	class __declspec(dllexport) AssetRegistry {
	public:
		AssetRegistry() = default;
		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry(const std::string& projectPath, AssetDirectoryWatcher* assetWatcher);
		void DeletePath(const std::string& relativePath);
		void RenamePath(const std::string& oldRelativePath, const std::string& newName);
		void MovePath(const std::string& oldRelativePath, const std::string& folderRelativePath);
	
		void AddMesh(boost::uuids::uuid guid, const ref<LuxonEngine::Mesh>& mesh);
		ref<LuxonEngine::Mesh> GetMesh(boost::uuids::uuid guid);
		void ImportAllAssets();
		void ImportAsset(const fs::path& filePath);
	private:
		std::string m_projectPath;
		AssetDirectoryWatcher* m_assetWatcher;

		std::map<boost::uuids::uuid, ref<LuxonEngine::Mesh>> m_meshes;
	};
}