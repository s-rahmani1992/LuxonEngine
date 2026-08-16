#include "AssetRegistry.h"
#include <filesystem>
#include <fstream>
#include <EngineAPI.h>
#include "EngineApplication.h"
#include <Core/SerializationStream.h>

#include "AssimpModel3DImporter.h"
#include "WICTextureImporter.h"
#include "MaterialImporter.h"
#include "AssetDirectoryWatcher.h"
#include "GuidUtilities.h"

namespace fs = std::filesystem;

LuxonEditor::AssetRegistry::AssetRegistry(const std::string& projectPath, AssetDirectoryWatcher* assetWatcher)
	:m_projectPath(projectPath), m_assetWatcher(assetWatcher)
{
	m_assetWatcher->RegisterCallback([this](const FileChangeEvent& event) {
		for (const auto& modified : event.modifiedFiles) {
			fs::path modifiedPath = fs::path(m_projectPath) / "Assets" / modified;
			if (fs::is_regular_file(modifiedPath)) {
				ImportAsset(modifiedPath);
			}
		}

		for (const auto& created : event.createdFiles) {
			fs::path createdPath = fs::path(m_projectPath) / "Assets" / created;
			if (fs::is_regular_file(createdPath)) {
				ImportAsset(createdPath);
			}
		};

		for (auto& deleted : event.deletedFiles) {
			fs::path deletedPath = fs::path(m_projectPath) / "Assets" / deleted;
			DeleteAsset(deletedPath);
		}
		});
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

void LuxonEditor::AssetRegistry::AddMesh(boost::uuids::uuid guid, const std::string& name, const ref<LuxonEngine::Mesh>& mesh)
{
	auto it = m_meshEntries.find(guid);

	if(it != m_meshEntries.end()) {
		InvokeMeshChangedCallbacks((*it).second.asset, mesh);
		(*it).second.asset->Release();
		EngineApplication::GetGPUApplication()->CreateAssetManager()->UnloadMesh((*it).second.asset);
		(*it).second.name = name;
		(*it).second.asset = mesh;
	}
	else {
		m_meshEntries.emplace(guid, AssetEntry<LuxonEngine::Mesh>{ mesh, guid, name });
	}
}

void LuxonEditor::AssetRegistry::AddTexture(boost::uuids::uuid guid, const std::string& name, const ref<LuxonEngine::Texture2D>& texture)
{
	auto it = m_textureEntries.find(guid);
	if(it != m_textureEntries.end()) {
		(*it).second.asset->Release();
		EngineApplication::GetGPUApplication()->CreateAssetManager()->UnloadTexture((*it).second.asset);
		(*it).second.asset = texture;
	}
	else {
		m_textureEntries.emplace(guid, AssetEntry<LuxonEngine::Texture2D>{ texture, guid, name });
	}
}

void LuxonEditor::AssetRegistry::AddMaterial(boost::uuids::uuid guid, const std::string& name, const ref<LuxonEngine::Rendering::Material>& material)
{
	auto it = m_materialEntries.find(guid);
	if(it != m_materialEntries.end()) {
		(*it).second.asset = material;
		(*it).second.name = name;
	}
	else {
		m_materialEntries.emplace(guid, AssetEntry<LuxonEngine::Rendering::Material>{ material, guid, name });
	}
}

ref<LuxonEngine::Mesh> LuxonEditor::AssetRegistry::GetMesh(boost::uuids::uuid guid)
{
	auto it = m_meshEntries.find(guid);
	return (it != m_meshEntries.end()) ? (*it).second.asset : nullptr;
}

ref<LuxonEngine::Texture2D> LuxonEditor::AssetRegistry::GetTexture(boost::uuids::uuid guid)
{
	auto it = m_textureEntries.find(guid);

	if(it != m_textureEntries.end()) {
		return (*it).second.asset;
	}
	return nullptr;
}

ref<LuxonEngine::Rendering::Material> LuxonEditor::AssetRegistry::GetMaterial(boost::uuids::uuid guid)
{
	auto it = m_materialEntries.find(guid);

	return (it != m_materialEntries.end()) ? (*it).second.asset : nullptr;
}

std::vector<LuxonEditor::AssetEntry<LuxonEngine::Texture2D>*> LuxonEditor::AssetRegistry::GetAllTextureEntries()
{
	std::vector<AssetEntry<LuxonEngine::Texture2D>*> entries;
	for (auto& [guid, entry] : m_textureEntries) {
		entries.push_back(&entry);
	}
	return entries;
}

LuxonEditor::AssetEntry<LuxonEngine::Texture2D>* LuxonEditor::AssetRegistry::GetTextureEntry(const ref<LuxonEngine::Texture2D> texture)
{
	auto it = std::find_if(m_textureEntries.begin(), m_textureEntries.end(), [&texture](const auto& pair) {
		return pair.second.asset == texture;
		});

	return (it != m_textureEntries.end()) ? &it->second : nullptr;
}

std::vector<LuxonEditor::AssetEntry<LuxonEngine::Mesh>*> LuxonEditor::AssetRegistry::GetAllMeshEntries()
{
	std::vector<AssetEntry<LuxonEngine::Mesh>*> entries;
	for (auto& [guid, entry] : m_meshEntries) {
		entries.push_back(&entry);
	}
	return entries;
}

std::vector<LuxonEditor::AssetEntry<LuxonEngine::Rendering::Material>*> LuxonEditor::AssetRegistry::GetAllMaterialEntries()
{
	std::vector<AssetEntry<LuxonEngine::Rendering::Material>*> entries;
	for (auto& [guid, entry] : m_materialEntries) {
		entries.push_back(&entry);
	}
	return entries;
}

LuxonEditor::AssetEntry<LuxonEngine::Mesh>* LuxonEditor::AssetRegistry::GetMeshEntry(const ref<LuxonEngine::Mesh> mesh)
{
	auto it = std::find_if(m_meshEntries.begin(), m_meshEntries.end(), [&mesh](const auto& pair) {
		return pair.second.asset == mesh;
		});

	return (it != m_meshEntries.end()) ? &it->second : nullptr;
}

LuxonEditor::AssetEntry<LuxonEngine::Rendering::Material>* LuxonEditor::AssetRegistry::GetMaterialEntry(const ref<LuxonEngine::Rendering::Material> material)
{
	auto it = std::find_if(m_materialEntries.begin(), m_materialEntries.end(), [&material](const auto& pair) {
		return pair.second.asset == material;
		});

	return (it != m_materialEntries.end()) ? &it->second : nullptr;
}

void LuxonEditor::AssetRegistry::ImportAllAssets()
{
	ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);
	AddMesh(GuidGenerator::GenerateGUIDFromString("585aa516-59c6-4fb4-a893-e4d56f65431a"), "Sphere", sphereMesh);

	ref<Mesh> cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);
	AddMesh(GuidGenerator::GenerateGUIDFromString("3a749398-71d0-4277-bb2d-dd01a5e140c4"), "Cube", cubeMesh);

	ref<Mesh> planeMesh = ShapeBuilder::CreatePlane(5.0f, 5.0f, 2, 2);
	AddMesh(GuidGenerator::GenerateGUIDFromString("837f517a-9689-4c3f-88a9-0042120f3abd"), "Plane", planeMesh);

	std::string assetPath = m_projectPath + "/Assets";
	std::string internalAssetPath = m_projectPath + "/Data/InternalAssets";
	// Iterate through all files in the assetPath directory and its subdirectories

	for (const auto& entry : fs::recursive_directory_iterator(assetPath)) {
		if (entry.is_regular_file()) {
			ImportAsset(entry.path());
		}
	}

	for(const auto& entry : fs::recursive_directory_iterator(internalAssetPath)) {
		if (entry.is_regular_file()) {
			ImportAsset(entry.path());
		}
	}

	for(auto [guid, textureEntry] : m_textureEntries) {
		EngineApplication::GetGPUApplication()->CreateAssetManager()->UploadTextureToGPU(textureEntry.asset);
	}

	for (const auto& entry : fs::recursive_directory_iterator(assetPath)) {
		if (entry.is_regular_file()) {
			ImportEngineAsset(entry.path());
		}
	}

	for(const auto& entry : fs::recursive_directory_iterator(internalAssetPath)) {
		if (entry.is_regular_file()) {
			ImportEngineAsset(entry.path());
		}
	}
}

static bool IsTextureExtension(const std::string& ext)
{
	return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tiff" || ext == ".tif";
}

void LuxonEditor::AssetRegistry::ImportAsset(const fs::path& path)
{
	std::string filePath = path.string();
	std::string extension = path.extension().string();
	// Check if the file is a mesh file based on its extension
	if (extension == ".obj" || extension == ".fbx") {
		// read file
		std::ifstream file(filePath, std::ios::binary);

		if (!file) {
			// Handle error opening the file
			return;
		}

		// Read the file contents into a buffer
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		SerializationStream stream;
		stream.LoadFromFile(filePath + ".json");

		// Import the mesh using AssimpModel3DImporter
		std::string error;
		auto model = AssimpModel3DImporter::Import(reinterpret_cast<const Byte*>(buffer.data()), buffer.size(), stream, this, error);

		if (model == nullptr) {
			Logger::LogError("Failed to import model: " + error);
			return;
		}
		}
	else if (IsTextureExtension(extension)) {
		std::ifstream file(filePath, std::ios::binary);
		if (!file) {
			return;
		}

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		SerializationStream stream;
		if (!stream.LoadFromFile(filePath + ".json")) {
			stream = WICTextureImporter::GenerateMetaFromFile(reinterpret_cast<const Byte*>(buffer.data()), buffer.size());
			stream.SaveToFile(filePath + ".json");
		}

		std::string error;
		auto texture = WICTextureImporter::Import(reinterpret_cast<const Byte*>(buffer.data()), buffer.size(), stream, path.filename().string(), error);

		if (texture == nullptr) {
			Logger::LogError("Failed to import texture: " + error);
			return;
		}

		EngineApplication::GetGPUApplication()->CreateAssetManager()->UploadTextureToGPU(texture);
	}
}

void LuxonEditor::AssetRegistry::ImportEngineAsset(const fs::path& path)
{
	std::string filePath = path.string();
	std::string extension = path.extension().string();

	std::string error;
	// Check if the file is a mesh file based on its extension
	if (extension == ".lmat") {
		std::ifstream file(filePath, std::ios::binary);

		if (!file) {
			// Handle error opening the file
			return;
		}

		// Read the file contents into a buffer
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		auto material = MaterialImporter::Import(reinterpret_cast<Byte*>(buffer.data()), buffer.size(), error);

		if (material == nullptr) {
			Logger::LogError("Failed to import material: " + error);
			return;
		}

		SerializationStream stream;
		stream.LoadFromFile(filePath + ".json");

		AddMaterial(stream.GetGuid("uuid"), path.filename().string(), material);
	}
}

void LuxonEditor::AssetRegistry::DeleteAsset(const fs::path& filePath)
{
	std::string filePathStr = filePath.string();
	std::string extension = filePath.extension().string();

	if(extension == ".obj" || extension == ".fbx") {
		// Load the metadata file to get the GUID
		SerializationStream stream;
		if (!stream.LoadFromFile(filePathStr + ".json")) {
			Logger::LogError("Failed to load metadata for asset: " + filePathStr);
			return;
		}

		std::vector<SerializationStream> meshesArray = stream.Array("meshes");

		for(auto& meshStream : meshesArray) {
			LuxonEngine::GUID meshGuid = meshStream.GetGuid("uuid");
			auto mesh = GetMesh(meshGuid);
			if(mesh) {
				InvokeMeshDeletedCallbacks(mesh);
				mesh->Release();
				EngineApplication::GetGPUApplication()->CreateAssetManager()->UnloadMesh(mesh);
				m_meshEntries.erase(meshGuid);
			}
		}
	}
	else if (IsTextureExtension(extension)) {
		SerializationStream stream;
		if (!stream.LoadFromFile(filePathStr + ".json")) {
			return;
		}

		LuxonEngine::GUID texGuid = stream.GetGuid("uuid");
		auto it = m_textureEntries.find(texGuid);
		if (it != m_textureEntries.end()) {
			it->second.asset->Release();
			UpdateDependentAssets(it->second.asset);
			EngineApplication::GetGPUApplication()->CreateAssetManager()->UnloadTexture(it->second.asset);
			m_textureEntries.erase(it);
		}
	}
	else if(extension == ".lmat") {
		SerializationStream stream;
		if (!stream.LoadFromFile(filePathStr + ".json")) {
			return;
		}
		LuxonEngine::GUID matGuid = stream.GetGuid("uuid");
		auto it = m_materialEntries.find(matGuid);
		if (it != m_materialEntries.end()) {
			InvokeMaterialDeletedCallbacks(it->second.asset);
			m_materialEntries.erase(it);
		}
	}
}

void LuxonEditor::AssetRegistry::ImportExternalFile(const std::string& sourceFilePath, const std::string& targetFolderPath)
{
	fs::path sourcePath(sourceFilePath);
	auto extention = sourcePath.extension().string();
	fs::path targetPath = (fs::path(m_projectPath) / "Assets" / targetFolderPath / sourcePath.filename()).lexically_normal();

	if(extention == ".obj" || extention == ".fbx") {
		fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing);
		
		std::ifstream file(sourceFilePath, std::ios::binary);
		// Read the file contents into a buffer
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		auto meta = AssimpModel3DImporter::GenerateMetaFromFile(reinterpret_cast<const Byte*>(buffer.data()), buffer.size());
		meta.SaveToFile(targetPath.string() + ".json");
	}
	else if (IsTextureExtension(extention)) {
		fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing);

		std::ifstream file(sourceFilePath, std::ios::binary);
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		auto meta = WICTextureImporter::GenerateMetaFromFile(reinterpret_cast<const Byte*>(buffer.data()), buffer.size());
		meta.SaveToFile(targetPath.string() + ".json");
	}
	else {
		Logger::LogError("Unsupported file type for import: " + extention);
	}
}

size_t LuxonEditor::AssetRegistry::RegisterMeshDeletedCallback(MeshDeletedCallback callback)
{
	size_t callbackId = ++m_lastmeshDeleteCallbackId;
	m_meshDeletedCallbacks[callbackId] = callback;
	return callbackId;
}

void LuxonEditor::AssetRegistry::UnregisterMeshDeletedCallback(size_t callbackId)
{
	m_meshDeletedCallbacks.erase(callbackId);
}

size_t LuxonEditor::AssetRegistry::RegisterMeshChangedCallback(MeshChangedCallback callback)
{
	auto callbackId = ++m_lastMeshChangedCallbackId;
	m_meshChangedCallbacks[callbackId] = callback;
	return callbackId;
}

void LuxonEditor::AssetRegistry::UnregisterMeshChangedCallback(size_t callbackId)
{
	m_meshChangedCallbacks.erase(callbackId);
}

size_t LuxonEditor::AssetRegistry::RegisterMaterialDeletedCallback(MaterialDeletedCallback callback)
{
	auto callbackId = ++m_lastMaterialDeletedCallbackId;
	m_materialDeletedCallbacks[callbackId] = callback;
	return callbackId;
}

void LuxonEditor::AssetRegistry::UnregisterMaterialDeletedCallback(size_t callbackId)
{
	m_materialDeletedCallbacks.erase(callbackId);
}

void LuxonEditor::AssetRegistry::InvokeMeshDeletedCallbacks(ref<LuxonEngine::Mesh>& mesh)
{
	for (auto& [id, callback] : m_meshDeletedCallbacks) {
		callback(mesh);
	}
}

void LuxonEditor::AssetRegistry::InvokeMeshChangedCallbacks(ref<LuxonEngine::Mesh>& oldMesh, const ref<LuxonEngine::Mesh>& newMesh)
{
	for(auto& [id, callback] : m_meshChangedCallbacks) {
		callback(oldMesh, newMesh);
	}
}

void LuxonEditor::AssetRegistry::InvokeMaterialDeletedCallbacks(ref<LuxonEngine::Rendering::Material>& material)
{
	for(auto& [id, callback] : m_materialDeletedCallbacks) {
		callback(material);
	}
}

void LuxonEditor::AssetRegistry::UpdateDependentAssets(const ref<LuxonEngine::Texture2D>& deletedTexture)
{
	for(auto & [guid, materialEntry] : m_materialEntries) {
		auto& material = materialEntry.asset;
		auto textureFields = material->GetTextureFields();
		for(auto& [fieldName, textureData] : *textureFields) {
			if(textureData.texture == deletedTexture) {
				material->SetTexture2D(fieldName, nullptr);
			}
		}
	}
}
