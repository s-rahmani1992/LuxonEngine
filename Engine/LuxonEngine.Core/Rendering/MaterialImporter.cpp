#include "MaterialImporter.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <StringUtilities.h>
#include <fstream>
#include <filesystem>
#include <Platform/Application.h>
#include <Core/AssetRegistry.h>
#include <Rendering/ShaderRegistery.h>
#include <Rendering/MaterialFactory.h>
#include <Rendering/Material.h>
#include "JSONUtilities.h"

ref<LuxonEngine::Rendering::Material> LuxonEngine::Rendering::MaterialImporter::ImportMaterial(const std::wstring& fileName, std::string& error)
{
	std::ifstream materialfile(WStringToString(fileName), std::ios::in | std::ios::binary);

	boost::json::value materialJson;

	if (!materialfile) {
		error = "file not found";
		return nullptr;
	}

	auto jsonMaterialStr = std::string(
		std::istreambuf_iterator<char>(materialfile),
		std::istreambuf_iterator<char>()
	);

	boost::system::error_code ec;
	materialJson = boost::json::parse(jsonMaterialStr, ec);

	auto gpuDevice = Platform::Application::GetGPUDevice();
	auto assetManager = Platform::Application::GetAssetRegistry();
	auto shaderRegistry = gpuDevice->CreateShaderRegistery();
	auto materialFactory = gpuDevice->CreateMaterialFactory();

	auto uidStr = materialJson.as_object()["program_id"].as_string().c_str();
	boost::uuids::string_generator gen;

	auto program = shaderRegistry->GetProgramByGUID(gen(uidStr));

	if (program == nullptr) {
		error = "Cannot find the program";
		return nullptr;
	}

	auto material = materialFactory->CreateMaterial(program);

	auto& materialFields = materialJson.as_object()["fields"].as_object();

	for (auto& field : materialFields) {
		std::string name = field.key();

		auto& value = field.value().as_object();

		std::string type = value["type"].as_string().c_str();

		if (type == "texture") {
			auto texture = assetManager->GetTexture2D(gen(value["data"].as_string().c_str()));
			material->SetTexture2D(name, texture);
			continue;
		}

		if (type == "uint") {
			UInt32 intData = value["data"].as_int64();
			material->SetValue(name, intData);
			continue;
		}

		if (type == "float") {
			Float floatData = value["data"].as_double();
			material->SetValue(name, floatData);
			continue;
		}

		if (type == "color") {
			Color colorData = JSONToColor(value["data"].as_object());
			material->SetValue(name, colorData);
			continue;
		}
	}

	std::ifstream metafile(WStringToString(fileName) + ".json", std::ios::in | std::ios::binary);

	boost::json::value metaValue;

	if (!metafile) {
		metaValue = CreateDefaultMetaData(fileName, error);
	}
	else {
		auto jsonMetaStr = std::string(
			std::istreambuf_iterator<char>(metafile),
			std::istreambuf_iterator<char>()
		);

		boost::system::error_code ec;
		metaValue = boost::json::parse(jsonMetaStr, ec);
	}

	auto metauuidStr = metaValue.as_object()["uuid"].as_string().c_str();
	Platform::Application::GetAssetRegistry()->RegisterMaterial(gen(metauuidStr), material);

	return material;
}

boost::json::value LuxonEngine::Rendering::MaterialImporter::CreateDefaultMetaData(const std::wstring& fileName, std::string& error)
{
	boost::json::value meta;
	auto& ob = meta.emplace_object();
	auto randomuuidGenerator = boost::uuids::random_generator();
	ob.emplace("uuid", boost::uuids::to_string(randomuuidGenerator()));
	std::ofstream file(WStringToString(fileName) + ".json");

	file << boost::json::serialize(meta);
	file.close();

	return meta;
}
