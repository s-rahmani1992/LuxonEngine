#include "EngineShaderRegistry.h"
#include "AssetDirectoryWatcher.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderProgram.h"
#include "Core/SerializationStream.h"
#include <fstream>
#include <sstream>
#include <EngineAPI.h>
#include <StringUtilities.h>
#include <boost/uuid/uuid_io.hpp>
#include "EngineApplication.h"


LuxonEditor::EngineShaderRegistry::EngineShaderRegistry(Render::ShaderCompiler* shaderCompiler, AssetDirectoryWatcher* assetWatcher)
	: ShaderRegistery(shaderCompiler), m_assetWatcher(assetWatcher), m_callbackID(0)
{
	m_callbackID = m_assetWatcher->RegisterCallback(
		[this](const FileChangeEvent& event) { this->OnAssetChanged(event); }
	);
}

LuxonEditor::EngineShaderRegistry::~EngineShaderRegistry()
{
	for (auto& [guid, programEntry] : m_registeredPrograms)
		delete programEntry.program;

	for (auto& [guid, programEntry] : m_internalPrograms)
		delete programEntry.program;
}

void LuxonEditor::EngineShaderRegistry::CompileAllShaders()
{
	const std::string& rootDir = m_assetWatcher->GetRootDirectory();
	fs::path rootPath(rootDir);

	for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		CompileAtPath(entry.path());
	}

	rootPath = fs::path(EngineApplication::GetProjectPath() + "/Data/InternalShaders/");

	for (const auto& entry : fs::directory_iterator(rootPath)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		CompileAtPath(entry.path());
	}
}

LuxonEngine::Rendering::ShaderProgram* LuxonEditor::EngineShaderRegistry::GetProgram(GUID guid)
{
	ShaderEntry* entry = GetShaderEntry(guid);
	return entry ? entry->program : nullptr;
}

LuxonEditor::ShaderEntry* LuxonEditor::EngineShaderRegistry::GetShaderEntry(GUID guid)
{
	auto shaderIT = m_registeredPrograms.find(guid);
	if (shaderIT != m_registeredPrograms.end()) {
		return &(*shaderIT).second;
	}

	shaderIT = m_internalPrograms.find(guid);
	if (shaderIT != m_internalPrograms.end()) {
		return &(*shaderIT).second;
	}

	return nullptr;
}

LuxonEditor::ShaderEntry* LuxonEditor::EngineShaderRegistry::GetShaderEntry(const LuxonEngine::Rendering::ShaderProgram* program)
{
	auto matchesProgram = [program](const auto& pair) { return pair.second.program == program; };

	auto shaderIT = std::find_if(m_registeredPrograms.begin(), m_registeredPrograms.end(), matchesProgram);
	if (shaderIT != m_registeredPrograms.end()) {
		return &(*shaderIT).second;
	}

	shaderIT = std::find_if(m_internalPrograms.begin(), m_internalPrograms.end(), matchesProgram);
	return (shaderIT != m_internalPrograms.end()) ? &(*shaderIT).second : nullptr;
}

std::vector<LuxonEditor::ShaderEntry*> LuxonEditor::EngineShaderRegistry::GetAllShaderEntries() const
{
	std::vector<ShaderEntry*> entries;
	for (const auto& [guid, entry] : m_registeredPrograms) {
		entries.push_back(const_cast<ShaderEntry*>(&entry));
	}
	return entries;
}

void LuxonEditor::EngineShaderRegistry::OnAssetChanged(const FileChangeEvent& event)
{
	for (auto& deleted : event.deletedFiles) {
		fs::path filePath = fs::path(m_assetWatcher->GetRootDirectory()) / (deleted);

		if(filePath.extension() != ".hlsl") {
			continue;
		}

		fs::path jsonPath = fs::path(m_assetWatcher->GetRootDirectory())/(deleted + ".json");

		LuxonEngine::SerializationStream metadataStream;
		if (!metadataStream.LoadFromFile(jsonPath.string())) {
			continue;
		}

		GUID programGuid = metadataStream.GetGuid("uuid");
		for (auto* programs : { &m_registeredPrograms, &m_internalPrograms }) {
			auto shaderIT = programs->find(programGuid);
			if (shaderIT == programs->end()) {
				continue;
			}

			InvokeShaderDeletedCallback(&(*shaderIT).second);
			RemoveShaderProgram((*shaderIT).second);
			delete (*shaderIT).second.program;
			programs->erase(shaderIT);
		}
	}

	for (auto& modified : event.modifiedFiles) {
		CompileAtPath(fs::path(m_assetWatcher->GetRootDirectory()) / (modified), true);
	}

	for(auto& created : event.createdFiles) {
		CompileAtPath(fs::path(m_assetWatcher->GetRootDirectory()) / (created));
	}
}

void LuxonEditor::EngineShaderRegistry::CompileAtPath(const fs::path& filePath, bool fireEvent)
{
	std::string extension = filePath.extension().string();

	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension != ".hlsl") {
		return;
	}

	fs::path jsonPath = fs::path(filePath.string() + ".json");

	if (!fs::exists(jsonPath)) {
		return; // Skip if no metadata file exists
	}

	std::ifstream hlslFile(filePath, std::ios::binary);
	if (!hlslFile.is_open()) {
		return;
	}

	std::string hlslContent((std::istreambuf_iterator<char>(hlslFile)),
		std::istreambuf_iterator<char>());
	hlslFile.close();

	LuxonEngine::SerializationStream metadataStream;
	if (!metadataStream.LoadFromFile(jsonPath.string())) {
		return;
	}

	Render::ShaderCompileProperties properties{};
	properties.folderPath = CharToString(filePath.parent_path().string().c_str());
	auto propertiesObject = metadataStream.Object("data");
	FillProperties(properties, propertiesObject);

	if (properties.usage == Render::ShaderUsage::Internal && properties.identifier.empty()) {
		LuxonEngine::Logger::LogError("Internal shader " + filePath.filename().string() + " has no identifier.");
		return;
	}

	if (properties.usage == Render::ShaderUsage::User && properties.name.empty()) {
		LuxonEngine::Logger::LogWarning("Shader " + filePath.filename().string() + " has no name. using the file name instead.");
		properties.name = filePath.stem().string();
	}

	std::string error;
	const UInt64 codeLength = hlslContent.length();
	Render::ShaderProgram* compiledProgram = m_shaderCompiler->CompileProgram(
		reinterpret_cast<const Byte*>(hlslContent.c_str()),
		codeLength,
		properties,
		error
	);

	const bool isInternal = properties.usage == Render::ShaderUsage::Internal;
	const std::string identifier = isInternal ? properties.identifier : std::string();
	auto& targetPrograms = isInternal ? m_internalPrograms : m_registeredPrograms;
	auto& otherPrograms = isInternal ? m_registeredPrograms : m_internalPrograms;

	GUID programGuid = metadataStream.GetGuid("uuid");

	// usage changed: move the entry to the other map. extracting the node keeps ShaderEntry pointers valid
	if (auto node = otherPrograms.extract(programGuid))
		targetPrograms.insert(std::move(node));

	auto shaderIT = targetPrograms.find(programGuid);

	if (shaderIT != targetPrograms.end()) {
		RemoveShaderProgram((*shaderIT).second);

		if((*shaderIT).second.program)
			delete (*shaderIT).second.program;

		(*shaderIT).second.name = filePath.filename().string();
		(*shaderIT).second.program = compiledProgram;
		(*shaderIT).second.compileError = error;
		(*shaderIT).second.identifier = identifier;

		if(fireEvent)
			InvokeShaderChangedCallback(&(*shaderIT).second);
	}
	else {
		targetPrograms[programGuid] = { programGuid, filePath.filename().string(), error, compiledProgram, identifier };
	}

	if (isInternal && compiledProgram) {
		if (GetShaderProgram(identifier) != nullptr)
			LuxonEngine::Logger::LogWarning("Internal shader identifier \"" + identifier + "\" is already in use. " + filePath.filename().string() + " replaces it.");

		AddShaderProgram(identifier, compiledProgram);
	}

	if(compiledProgram == nullptr) {
		LuxonEngine::Logger::LogError("Error compiling " + filePath.filename().string() + ":\n " + error);
	}
}

void LuxonEditor::EngineShaderRegistry::RemoveShaderProgram(const ShaderEntry& entry)
{
	// only remove the name if it still points to this entry's program. another shader may have taken the identifier
	if (!entry.identifier.empty() && entry.program && GetShaderProgram(entry.identifier) == entry.program)
		m_namedPrograms.erase(entry.identifier);
}

void LuxonEditor::EngineShaderRegistry::InvokeShaderChangedCallback(ShaderEntry* entry)
{
	for (auto& [id, callback] : m_programChangedCallbacks) {
		callback(entry);
	}
}

void LuxonEditor::EngineShaderRegistry::FillProperties(LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& dataNode)
{
	dataNode.GetString("model", properties.model);

	std::string usageStr;
	dataNode.GetString("usage", usageStr);
	properties.usage = usageStr == "Internal" ? Render::ShaderUsage::Internal : Render::ShaderUsage::User;
	dataNode.GetString("identifier", properties.identifier);
	dataNode.GetString("name", properties.name);

	std::string typeStr;
	dataNode.GetString("type", typeStr);

	if (typeStr == "RayTracing") {
		properties.type = Render::ShaderProgramType::RayTracing;
		dataNode.GetString("rayGen", &properties.rayTracingProperties.rayGen);
		dataNode.GetString("miss", &properties.rayTracingProperties.miss);
		dataNode.GetString("intersection", &properties.rayTracingProperties.intersection);
		dataNode.GetString("anyHit", &properties.rayTracingProperties.anyHit);
		dataNode.GetString("closestHit", &properties.rayTracingProperties.closestHit);
	}
	else if (typeStr == "Rasterization") {
		properties.type = Render::ShaderProgramType::Rasterization;
		dataNode.GetString("vsMain", &properties.rasterProperties.vertexMain);
		dataNode.GetString("psMain", &properties.rasterProperties.pixelMain);
		dataNode.GetString("gsMain", &properties.rasterProperties.geometryMain);
	}
	else if (typeStr == "Compute") {
		properties.type = Render::ShaderProgramType::Compute;
		dataNode.GetString("csMain", &properties.computeProperties.computeMain);
	}
}

void LuxonEditor::EngineShaderRegistry::SerializeProperties(const LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream)
{
	stream.Clear();
	stream.SetString("model", properties.model);

	if (properties.usage == Render::ShaderUsage::Internal) {
		stream.SetString("usage", "Internal");
		stream.SetString("identifier", properties.identifier);
	}
	else {
		stream.SetString("usage", "User");
		stream.SetString("name", properties.name);
	}

	switch(properties.type) {
		case Render::ShaderProgramType::RayTracing:
			stream.SetString("type", "RayTracing");
			if(properties.rayTracingProperties.rayGen != nullptr )
				stream.SetString("rayGen", properties.rayTracingProperties.rayGen);
			if(properties.rayTracingProperties.miss != nullptr )
				stream.SetString("miss", properties.rayTracingProperties.miss);
			if(properties.rayTracingProperties.intersection != nullptr )
				stream.SetString("intersection", properties.rayTracingProperties.intersection);
			if(properties.rayTracingProperties.anyHit != nullptr )
				stream.SetString("anyHit", properties.rayTracingProperties.anyHit);
			if(properties.rayTracingProperties.closestHit != nullptr )
				stream.SetString("closestHit", properties.rayTracingProperties.closestHit);
			break;
		case Render::ShaderProgramType::Rasterization:
			stream.SetString("type", "Rasterization");
			stream.SetString("vsMain", properties.rasterProperties.vertexMain);
			stream.SetString("psMain", properties.rasterProperties.pixelMain);
			if(properties.rasterProperties.geometryMain != nullptr)
				stream.SetString("gsMain", properties.rasterProperties.geometryMain);
			break;
		case Render::ShaderProgramType::Compute:
			stream.SetString("type", "Compute");
			stream.SetString("csMain", properties.computeProperties.computeMain);
			break;
	}
}

size_t LuxonEditor::EngineShaderRegistry::RegisterShaderChangedCallback(ShaderProgramChangedCallback callback)
{
	size_t callbackId = ++m_lastProgramChangedCallbackId;
	m_programChangedCallbacks[callbackId] = callback;
	return callbackId;
}

void LuxonEditor::EngineShaderRegistry::UnregisterShaderChangedCallback(size_t callbackId)
{
	m_programChangedCallbacks.erase(callbackId);
}

size_t LuxonEditor::EngineShaderRegistry::RegisterShaderDeletedCallback(ShaderProgramDeletedCallback callback)
{
	auto callbackId = ++m_lastProgramDeletedCallbackId;
	m_programDeletedCallbacks[callbackId] = callback;
	return callbackId;
}

void LuxonEditor::EngineShaderRegistry::UnregisterShaderDeletedCallback(size_t callbackId)
{
	m_programDeletedCallbacks.erase(callbackId);
}

void LuxonEditor::EngineShaderRegistry::InvokeShaderDeletedCallback(ShaderEntry* entry)
{
	for (auto& [id, callback] : m_programDeletedCallbacks) {
		callback(entry);
	}
}
