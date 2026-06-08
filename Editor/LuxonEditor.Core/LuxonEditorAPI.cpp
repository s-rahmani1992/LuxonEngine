#include "LuxonEditorAPI.h"

LuxonEditor::EngineApplication* CreateEngineApplication(const LuxonEditor::ApplicationConfig& config, std::string& errorString)
{
	return LuxonEditor::EngineApplication::CreateApplication(config, errorString);
}

std::string GetProjectPath()
{
	return LuxonEditor::EngineApplication::GetProjectPath();
}

LuxonEditor::AssetRegistry* GetAssetManager()
{
	return LuxonEditor::EngineApplication::GetAssetManager();
}

void AssetRegistry_DeletePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& relativePath)
{
	assetRegistry->DeletePath(relativePath);
}

void AssetRegistry_RenamePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& oldRelativePath, const std::string& newName)
{
	assetRegistry->RenamePath(oldRelativePath, newName);
}

void AssetRegistry_MovePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& oldRelativePath, const std::string& folderRelativePath)
{
	assetRegistry->MovePath(oldRelativePath, folderRelativePath);
}
