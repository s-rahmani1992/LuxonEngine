#pragma once
#include <EngineAPI.h>
#include <string>

namespace LuxonEngine {
	class Model3DAsset;
	class SerializationStream;
}

using namespace LuxonEngine;

namespace LuxonEditor {
	class AssetRegistry;

	struct __declspec(dllexport) ModelImportProperties {
		Vector3 position = Vector3(0.0f);
		Vector3 axis = Vector3(0.0f, 0.0f, 1.0f);
		Float angleDeg = 0.0f;
		Vector3 scale = Vector3(1.0f);
	};

	class __declspec(dllexport) AssimpModel3DImporter
	{
	public:
		static ref<Model3DAsset> Import(const Byte* data, long size, SerializationStream& stream, AssetRegistry* assetManager, std::string& error);
		static void FillPropertiesFromStream(SerializationStream* stream, ModelImportProperties& properties);
		static void SerializePropertiesToStream(const ModelImportProperties& properties, SerializationStream* stream);
		static SerializationStream GenerateMetaFromFile(const Byte* data, long size);
	};
}