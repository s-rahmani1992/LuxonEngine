#pragma once
#include <string>
#include <imgui/imgui.h>
#include <map>
#include <wincodec.h>
#include <d3d11.h>

#define FOLDER_ICON_ID "folder_icon_id"
#define TEXTURE_ICON_ID "texture_icon_id"
#define HLSL_ICON_ID "hlsl_icon_id"
#define DEFAULT_ICON_ID "default_icon_id"
#define MODEL_ICON_ID "model_icon_id"
#define LOG_ICON_ID "log_icon_id"
#define WARN_ICON_ID "warn_icon_id"
#define ERROR_ICON_ID "error_icon_id"

namespace LuxonEditor {
	class AssetManager {
	public:
		AssetManager() = default;
		AssetManager(ID3D11Device* device);
		~AssetManager();
		void ImportIcon(const std::string& path, const std::string& idName);
		ImTextureID GetIcon(const std::string& idName);
		ImTextureID GetIconFromExtention(const std::string& extName);
	private:
		std::map<std::string, ImTextureID> m_icons;

		IWICImagingFactory* m_wicImageFactory = nullptr;
		ID3D11Device* m_device;
	};
}