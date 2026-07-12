#include "AssetRegistry.h"
#include <filesystem>
#include <fstream>
#include <EngineAPI.h>
#include <Core/SerializationStream.h>

#include "AssimpModel3DImporter.h"

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

void LuxonEditor::AssetRegistry::AddMesh(boost::uuids::uuid guid, const ref<LuxonEngine::Mesh>& mesh)
{
	auto it = m_meshes.find(guid);

	if(it != m_meshes.end()) {
		(*it).second = mesh;
	}
	else {
		m_meshes.emplace(guid, mesh);
	}
}

void LuxonEditor::AssetRegistry::ImportAllAssets()
{
	std::string assetPath = m_projectPath + "/Assets";

	// Iterate through all files in the assetPath directory and its subdirectories

	for (const auto& entry : fs::recursive_directory_iterator(assetPath)) {
		if (entry.is_regular_file()) {
			std::string filePath = entry.path().string();
			std::string extension = entry.path().extension().string();
			// Check if the file is a mesh file based on its extension
			if (extension == ".obj" || extension == ".fbx" || extension == ".gltf") {
				// read file
				std::ifstream file(filePath, std::ios::binary);

				if (!file) {
					// Handle error opening the file
					continue;
				}

				// Read the file contents into a buffer
				std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				file.close();

				SerializationStream stream;
				stream.LoadFromFile(filePath + ".json");

				// Import the mesh using AssimpModel3DImporter
				std::string error;
				auto model = AssimpModel3DImporter::Import(reinterpret_cast<const Byte*>(buffer.data()), buffer.size(), stream, this, error);
			
				if(model == nullptr) {
					Logger::LogError("Failed to import model: " + error);
					// Handle error importing the model
					continue;
				}
			}
		}
	}
}
