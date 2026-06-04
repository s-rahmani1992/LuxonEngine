#include "LuxonEditorAPI.h"

LuxonEditor::EngineApplication* CreateEngineApplication(const LuxonEditor::ApplicationConfig& config, std::string& errorString)
{
	return LuxonEditor::EngineApplication::CreateApplication(config, errorString);
}

LUXON_ENGINE_API std::string GetProjectPath()
{
	return LuxonEditor::EngineApplication::GetProjectPath();
}

LUXON_ENGINE_API LuxonEditor::AssetRegistry* GetAssetManager()
{
	return LuxonEditor::EngineApplication::GetAssetManager();
}

LUXON_ENGINE_API void AssetRegistry_DeletePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& relativePath)
{
	assetRegistry->DeletePath(relativePath);
}
