#pragma once
#include <BasicTypes.h>
#include <string>
#include <boost/json.hpp>
#include <Core/SerializationStream.h>

namespace LuxonEngine::Rendering {
	class Material;
}

namespace LuxonEditor {

	class MaterialImporter {
	public:
		static ref<LuxonEngine::Rendering::Material> Import(Byte* data, UInt64 size, std::string& error);
	
	private:
		static LuxonEngine::SerializationStream CreateDefaultMetaData();
	};
}