#pragma once
#include <BasicTypes.h>
#include <string>
#include <boost/json.hpp>
#include <Core/SerializationStream.h>

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEditor {

	class __declspec(dllexport) MaterialImporter {
	public:
		static ref<LuxonEngine::Rendering::Material> Import(Byte* data, UInt64 size, std::string& error);
		static void SerializeMaterial(const ref<LuxonEngine::Rendering::Material>& material, LuxonEngine::SerializationStream& stream);
		static LuxonEngine::SerializationStream CreateDefaultMetaData();
	};
}