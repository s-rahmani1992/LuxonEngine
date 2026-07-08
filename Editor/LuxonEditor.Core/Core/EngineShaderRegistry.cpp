#include "EngineShaderRegistry.h"
#include "AssetDirectoryWatcher.h"
#include "Rendering/ShaderRegistery.h"
#include "Rendering/ShaderProgram.h"
#include "Core/SerializationStream.h"
#include <fstream>
#include <sstream>
#include <EngineAPI.h>
#include <StringUtilities.h>
#include <boost/uuid/uuid_io.hpp>


LuxonEditor::EngineShaderRegistry::EngineShaderRegistry(Render::ShaderRegistery* shaderCompiler, AssetDirectoryWatcher* assetWatcher)
	: m_shaderCompiler(shaderCompiler), m_assetWatcher(assetWatcher), m_callbackID(0)
{
	m_callbackID = m_assetWatcher->RegisterCallback(
		[this](const FileChangeEvent& event) { this->OnAssetChanged(event); }
	);
}

LuxonEditor::EngineShaderRegistry::~EngineShaderRegistry()
{
	for (auto& [guid, program] : m_registeredPrograms)
		delete program;
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
}

void LuxonEditor::EngineShaderRegistry::OnAssetChanged(const FileChangeEvent& event)
{
	for (auto& deleted : event.deletedFiles) {
		fs::path jsonPath = fs::path(m_assetWatcher->GetRootDirectory())/(deleted + ".json");

		LuxonEngine::SerializationStream metadataStream;
		if (!metadataStream.LoadFromFile(jsonPath.string())) {
			continue;
		}

		GUID programGuid = metadataStream.GetGuid("uuid");
		auto shaderIT = m_registeredPrograms.find(programGuid);
		if (shaderIT != m_registeredPrograms.end()) {
			delete (*shaderIT).second;
			m_registeredPrograms.erase(shaderIT);
		}
	}

	for (auto& modified : event.modifiedFiles) {
		CompileAtPath(fs::path(m_assetWatcher->GetRootDirectory()) / (modified));
	}

	for(auto& created : event.createdFiles) {
		CompileAtPath(fs::path(m_assetWatcher->GetRootDirectory()) / (created));
	}
}

void LuxonEditor::EngineShaderRegistry::CompileAtPath(const fs::path& filePath)
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

	std::string error;
	const UInt64 codeLength = hlslContent.length();
	Render::ShaderProgram* compiledProgram = m_shaderCompiler->CompileProgram(
		reinterpret_cast<const Byte*>(hlslContent.c_str()),
		codeLength,
		properties,
		error
	);

	GUID programGuid = metadataStream.GetGuid("uuid");
	auto shaderIT = m_registeredPrograms.find(programGuid);

	if (compiledProgram == nullptr) {
		if (shaderIT != m_registeredPrograms.end()) {
			delete (*shaderIT).second;
			m_registeredPrograms.erase(shaderIT);
		}
		LuxonEngine::Logger::LogError(error);
		return;
	}
	
	if (shaderIT != m_registeredPrograms.end()) {
		delete (*shaderIT).second;
		(*shaderIT).second = compiledProgram;
	}
	else {
		m_registeredPrograms[programGuid] = compiledProgram;
	}
}

void LuxonEditor::EngineShaderRegistry::FillProperties(LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& dataNode)
{
	dataNode.GetString("model", properties.model);
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
