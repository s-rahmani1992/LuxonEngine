#pragma once
#include "../BasicTypes.h"
#include <Platform/CommonWin.h>
#include <string>
#include "GUIDUtility.h"
#include <wincodec.h>
#include <boost/json.hpp>

namespace LuxonEngine {
	class Texture2D;
	enum class TextureFormat;

	class WICTexture2DImporter {
	public:
		static ref<Texture2D> Import(const std::wstring& fileName, std::string& error);

	private:
		static boost::json::value CreateDefaultMeta(const std::wstring& fileName, std::string& error);

		const static GuidMap<TextureFormat> m_texFormatMaps;
		const static GuidMap<GUID> m_convertFormatMaps;
	};
}