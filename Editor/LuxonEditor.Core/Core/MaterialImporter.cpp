#include "MaterialImporter.h"

#include <StringUtilities.h>
#include <fstream>
#include <filesystem>
#include <Platform/Application.h>
#include <Rendering/ShaderRegistery.h>
#include <Rendering/MaterialFactory.h>
#include <Rendering/Material.h>
#include "JSONUtilities.h"
#include "EngineApplication.h"
#include "EngineShaderRegistry.h"
#include "AssetRegistry.h"
#include "GuidUtilities.h"

ref<LuxonEngine::Rendering::Material> LuxonEditor::MaterialImporter::Import(Byte* data, UInt64 size, std::string& error)
{
	LuxonEngine::SerializationStream materialStream;
	materialStream.LoadFromMemory(data, size);

	auto gpuDevice = EngineApplication::GetGPUApplication();
	auto assetManager = EngineApplication::GetAssetManager();
	auto shaderRegistry = EngineApplication::GetShaderRegistery();
	auto materialFactory = gpuDevice->CreateMaterialFactory();

	auto programId = materialStream.GetGuid("program_id");

	auto program = shaderRegistry->GetProgram(programId);

	if (program == nullptr) {
		error = "Cannot find the program";
		return nullptr;
	}

	auto material = materialFactory->CreateMaterial(std::shared_ptr<LuxonEngine::Rendering::ShaderProgram>(program));

	auto materialFieldStream = materialStream.Object("fields");

	auto valueFields = materialFieldStream.GetObjectFields();

	for(auto& [fieldName, fieldStream] : valueFields) {
		std::string type;
		fieldStream.GetString("type", type);

		if (type == "texture") {
			auto texture = assetManager->GetTexture(fieldStream.GetGuid("data"));
			material->SetTexture2D(fieldName, texture);
			continue;
		}

		if (type == "uint") {
			UInt32 intData = static_cast<UInt32>(fieldStream.GetInt("data", 0));
			material->SetValue(fieldName, intData);
			continue;
		}

		if (type == "float") {
			Float floatData = fieldStream.GetFloat("data", 0.0f);
			material->SetValue(fieldName, floatData);
			continue;
		}

		if (type == "color") {
			LuxonEngine::Color color;
			fieldStream.GetColor("data", color);
			material->SetValue(fieldName, color);
			continue;
		}
	}

	return material;
}

void LuxonEditor::MaterialImporter::SerializeMaterial(const ref<LuxonEngine::Rendering::Material>& material, LuxonEngine::SerializationStream& stream)
{
	auto program = material->GetProgram();
	auto entry = EngineApplication::GetShaderRegistery()->GetShaderEntry(program.get());
	stream.SetGuid("program_id", entry->guid);
	LuxonEngine::SerializationStream fieldListStream;
	auto valueFields = material->GetValueFields();

	for(auto& [fieldName, valueData] : *valueFields) {
		LuxonEngine::SerializationStream fieldStream;

		switch(valueData.size) {
			case sizeof(Float):
				fieldStream.SetFloat("data", *(Float*)valueData.data);
				fieldStream.SetString("type", "float");
				break;
				case sizeof(LuxonEngine::Color) : 
				{
					Float* ptr = (Float*)valueData.data;
					LuxonEngine::Color color(*ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
					fieldStream.SetColor("data", color);
					fieldStream.SetString("type", "color");
					break; 
				}
			default:
				continue; // Skip unsupported types
		}

		fieldListStream.SetObject(fieldName, fieldStream);
	}

	auto textureFields = material->GetTextureFields();

	for(auto& [fieldName, textureData] : *textureFields) {
		LuxonEngine::SerializationStream fieldStream;
		fieldStream.SetGuid("data", textureData.texture ? EngineApplication::GetAssetManager()->GetTextureEntry(textureData.texture)->guid : LuxonEditor::GuidGenerator::GenerateGUID());
		fieldStream.SetString("type", "texture");
		fieldListStream.SetObject(fieldName, fieldStream);
	}

	stream.SetObject("fields", fieldListStream);
}

LuxonEngine::SerializationStream LuxonEditor::MaterialImporter::CreateDefaultMetaData()
{
	LuxonEngine::SerializationStream stream;
	stream.SetGuid("uuid", LuxonEditor::GuidGenerator::GenerateGUID());
	return stream;
}
