#include "LuxonEditorAPI.h"
#include "Core/ShaderCreator.h"

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

LUXON_ENGINE_API LuxonEditor::SelectionManager* GetSelectionManager()
{
	return LuxonEditor::EngineApplication::GetSelectionManager();
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

LUXON_ENGINE_API void CreateShader(const LuxonEngine::Rendering::ShaderCompileProperties& properties, const std::string& shaderName)
{
	LuxonEditor::ShaderCreator::CreateShader(properties, shaderName);
}
