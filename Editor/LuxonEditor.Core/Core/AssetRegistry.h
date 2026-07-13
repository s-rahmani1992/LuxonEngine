#pragma once
#include <string>
#include <map>
#include <EngineAPI.h>
#include <boost/uuid/uuid.hpp>

namespace LuxonEditor {
	class __declspec(dllexport) AssetRegistry {
	public:
		AssetRegistry() = default;
		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry(const std::string& projectPath);
		void DeletePath(const std::string& relativePath);
		void RenamePath(const std::string& oldRelativePath, const std::string& newName);
		void MovePath(const std::string& oldRelativePath, const std::string& folderRelativePath);
	
		void AddMesh(boost::uuids::uuid guid, const ref<LuxonEngine::Mesh>& mesh);
		ref<LuxonEngine::Mesh> GetMesh(boost::uuids::uuid guid);
		void ImportAllAssets();
	private:
		std::string m_projectPath;

		std::map<boost::uuids::uuid, ref<LuxonEngine::Mesh>> m_meshes;
	};
}