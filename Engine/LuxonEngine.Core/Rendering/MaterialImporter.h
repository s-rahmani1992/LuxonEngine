#pragma once
#include <BasicTypes.h>
#include <string>
#include <boost/json.hpp>

namespace LuxonEngine::Rendering {
	class Material;

	class MaterialImporter {
	public:
		static ref<Material> ImportMaterial(const std::wstring& fileName, std::string& error);
	
	private:
		static boost::json::value CreateDefaultMetaData(const std::wstring& fileName, std::string& error);
	};
}