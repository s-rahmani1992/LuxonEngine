#pragma once
#include <string>
#include <map>
#include <EngineAPI.h>
#include <boost/uuid/uuid.hpp>
#include <filesystem>

namespace fs = std::filesystem;

namespace LuxonEditor {
	class AssetDirectoryWatcher;

	template <typename T>
	struct AssetEntry {
		ref<T> asset;
		boost::uuids::uuid guid;
		std::string name;
	};

	class __declspec(dllexport) AssetRegistry {
	public:
		using MeshDeletedCallback = std::function<void(ref<LuxonEngine::Mesh>&)>;
		using MeshChangedCallback = std::function<void(ref<LuxonEngine::Mesh>&, const ref<LuxonEngine::Mesh>&)>;
		using MaterialDeletedCallback = std::function<void(ref<LuxonEngine::Rendering::Material>&)>;

		AssetRegistry() = default;
		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry(const std::string& projectPath, AssetDirectoryWatcher* assetWatcher);
		void DeletePath(const std::string& relativePath);
		void RenamePath(const std::string& oldRelativePath, const std::string& newName);
		void MovePath(const std::string& oldRelativePath, const std::string& folderRelativePath);

		void AddMesh(boost::uuids::uuid guid, const std::string& name, const ref<LuxonEngine::Mesh>& mesh);
		void AddTexture(boost::uuids::uuid guid, const std::string& name, const ref<LuxonEngine::Texture2D>& texture);
		void AddMaterial(boost::uuids::uuid guid, const std::string& name, const ref<LuxonEngine::Rendering::Material>& material);
		ref<LuxonEngine::Mesh> GetMesh(boost::uuids::uuid guid);
		ref<LuxonEngine::Texture2D> GetTexture(boost::uuids::uuid guid);
		ref<LuxonEngine::Rendering::Material> GetMaterial(boost::uuids::uuid guid);

		std::vector<AssetEntry<LuxonEngine::Texture2D>*> GetAllTextureEntries();
		std::vector<AssetEntry<LuxonEngine::Mesh>*> GetAllMeshEntries();
		std::vector<AssetEntry<LuxonEngine::Rendering::Material>*> GetAllMaterialEntries();
		AssetEntry<LuxonEngine::Texture2D>* GetTextureEntry(const ref<LuxonEngine::Texture2D> texture);
		AssetEntry<LuxonEngine::Mesh>* GetMeshEntry(const ref<LuxonEngine::Mesh> mesh);
		AssetEntry<LuxonEngine::Rendering::Material>* GetMaterialEntry(const ref<LuxonEngine::Rendering::Material> material);
		void ImportAllAssets();
		void ImportAsset(const fs::path& filePath);
		void ImportEngineAsset(const fs::path& filePath);
		void DeleteAsset(const fs::path& filePath);
		void ImportExternalFile(const std::string& sourcePath, const std::string& targetRelativePath);
		size_t RegisterMeshDeletedCallback(MeshDeletedCallback callback);
		void UnregisterMeshDeletedCallback(size_t callbackId);
		size_t RegisterMeshChangedCallback(MeshChangedCallback callback);
		void UnregisterMeshChangedCallback(size_t callbackId);
		size_t RegisterMaterialDeletedCallback(MaterialDeletedCallback callback);
		void UnregisterMaterialDeletedCallback(size_t callbackId);
	private:
		void UpdateDependentAssets(const ref<LuxonEngine::Texture2D>& texture);
		void InvokeMeshDeletedCallbacks(ref<LuxonEngine::Mesh>& mesh);
		void InvokeMeshChangedCallbacks(ref<LuxonEngine::Mesh>& oldMesh, const ref<LuxonEngine::Mesh>& newMesh);
		void InvokeMaterialDeletedCallbacks(ref<LuxonEngine::Rendering::Material>& material);
		std::string m_projectPath;
		AssetDirectoryWatcher* m_assetWatcher;

		std::map<boost::uuids::uuid, AssetEntry<LuxonEngine::Rendering::Material>> m_materialEntries;
		std::map<boost::uuids::uuid, AssetEntry<LuxonEngine::Texture2D>> m_textureEntries;
		std::map<boost::uuids::uuid, AssetEntry<LuxonEngine::Mesh>> m_meshEntries;

		std::map<size_t, MeshDeletedCallback> m_meshDeletedCallbacks;
		size_t m_lastmeshDeleteCallbackId = 0;

		std::map<size_t, MeshChangedCallback> m_meshChangedCallbacks;
		size_t m_lastMeshChangedCallbackId = 0;

		std::map<size_t, MaterialDeletedCallback> m_materialDeletedCallbacks;
		size_t m_lastMaterialDeletedCallbackId = 0;
	};
}