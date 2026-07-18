#pragma once
#include <EngineAPI.h>
#include <string>

namespace LuxonEngine {
	class Texture2D;
	class SerializationStream;
}

namespace LuxonEditor {
	class AssetRegistry;
	using Guid = boost::uuids::uuid;

	class __declspec(dllexport) WICTextureImporter
	{
	public:
		static ref<LuxonEngine::Texture2D> Import(const Byte* data, long size, LuxonEngine::SerializationStream& stream, AssetRegistry* assetRegistry, std::string& error);
		static LuxonEngine::SerializationStream GenerateMetaFromFile(const Byte* data, long size);
	};
}