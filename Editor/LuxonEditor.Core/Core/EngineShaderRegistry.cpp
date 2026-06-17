#include "EngineShaderRegistry.h"
#include "AssetDirectoryWatcher.h"
#include "Rendering/ShaderRegistery.h"
#include "Rendering/ShaderProgram.h"
#include "Core/SerializationStream.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <EngineAPI.h>
#include <StringUtilities.h>

namespace fs = std::filesystem;
namespace Render = LuxonEngine::Rendering;

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

		fs::path filePath = entry.path();
		std::string extension = filePath.extension().string();

		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		if (extension != ".hlsl") {
			continue;
		}

		fs::path jsonPath = fs::path(filePath.string() + ".json");

		if (!fs::exists(jsonPath)) {
			continue; // Skip if no metadata file exists
		}

		std::ifstream hlslFile(filePath, std::ios::binary);
		if (!hlslFile.is_open()) {
			continue;
		}

		std::string hlslContent((std::istreambuf_iterator<char>(hlslFile)),
								std::istreambuf_iterator<char>());
		hlslFile.close();

		LuxonEngine::SerializationStream metadataStream;
		if (!metadataStream.LoadFromFile(jsonPath.string())) {
			continue;
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

		if (compiledProgram != nullptr) {
			GUID programGuid = metadataStream.GetGuid("uuid");
			auto shaderIT = m_registeredPrograms.find(programGuid);
			if (shaderIT != m_registeredPrograms.end()) {
				delete (*shaderIT).second;
				(*shaderIT).second = compiledProgram;
			}
			else {
				m_registeredPrograms[programGuid] = compiledProgram;
			}
		}
		else {
			LuxonEngine::Logger::LogError(error);
		}
	}
}

void LuxonEditor::EngineShaderRegistry::OnAssetChanged(const FileChangeEvent& event)
{
	
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
