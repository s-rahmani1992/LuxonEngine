#pragma once
#include "Texture2D.h"
#include "../BasicTypes.h"
#include <string>
#include "GUIDUtility.h"
#include <vector>
#include <wincodec.h>

#pragma comment(lib, "WindowsCodecs.lib")

namespace QuantumEngine {
	class WICTexture2DImporter {
	public:
		static ref<Texture2D> Import(const std::wstring& fileName, std::string& error);
	private:
		const static GuidMap<TextureFormat> m_texFormatMaps;
		const static GuidMap<GUID> m_convertFormatMaps;
	};
}