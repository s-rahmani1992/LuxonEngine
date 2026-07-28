#pragma once
#include <EngineAPI.h>
#include "Core/EngineApplication.h"
#include "Core/AssetRegistry.h"
#include "Core/SelectionManager.h"
#include "Core/AssimpModel3DImporter.h"
#include "Core/EngineShaderRegistry.h"
#include "Core/GuidUtilities.h"

LUXON_ENGINE_API LuxonEditor::EngineApplication* CreateEngineApplication(const LuxonEditor::ApplicationConfig& config, std::string& errorString);

LUXON_ENGINE_API std::string GetProjectPath();

LUXON_ENGINE_API LuxonEngine::Rendering::GPUDeviceManager* GetGPUApplication();

LUXON_ENGINE_API LuxonEditor::AssetRegistry* GetAssetManager();

LUXON_ENGINE_API LuxonEditor::SelectionManager* GetSelectionManager();

LUXON_ENGINE_API void AssetRegistry_DeletePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& relativePath);

LUXON_ENGINE_API void AssetRegistry_RenamePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& oldRelativePath, const std::string& newName);

LUXON_ENGINE_API void AssetRegistry_MovePath(LuxonEditor::AssetRegistry* assetRegistry, const std::string& oldRelativePath, const std::string& folderRelativePath);

LUXON_ENGINE_API void CreateShader(const LuxonEngine::Rendering::ShaderCompileProperties& properties, const std::string& shaderName);