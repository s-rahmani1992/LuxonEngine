#pragma once
#include <EngineAPI.h>
#include "Core/EngineApplication.h"
#include "Core/AssetRegistry.h"

LUXON_ENGINE_API LuxonEditor::EngineApplication* CreateEngineApplication(const LuxonEditor::ApplicationConfig& config, std::string& errorString);

LUXON_ENGINE_API std::string GetProjectPath();

LUXON_ENGINE_API LuxonEditor::AssetRegistry* GetAssetManager();

LUXON_ENGINE_API void AssetRegistry_DeletePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& relativePath);

LUXON_ENGINE_API void AssetRegistry_RenamePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& oldRelativePath, const std::string& newName);
