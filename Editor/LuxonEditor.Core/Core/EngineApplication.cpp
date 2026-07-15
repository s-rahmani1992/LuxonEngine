#include "EngineApplication.h"
#include "AssetRegistry.h"
#include "AssetDirectoryWatcher.h"
#include <Core/SerializationStream.h>
#include <EngineAPI.h>
#include <StringUtilities.h>
#include "EngineShaderRegistry.h"
#include "SelectionManager.h"
#include <Rendering/ShaderRegistery.h>

LuxonEditor::EngineApplication LuxonEditor::EngineApplication::m_appInstance;

LuxonEditor::EngineApplication* LuxonEditor::EngineApplication::CreateApplication(const ApplicationConfig& config, std::string& error)
{
	m_appInstance = EngineApplication(config);

	if (m_appInstance.Initialize(error) == false)
		return nullptr;

    return &m_appInstance;
}

LuxonEditor::SelectionManager* LuxonEditor::EngineApplication::GetSelectionManager()
{
	return m_appInstance.m_selectionManager;
}

LuxonEngine::Rendering::GPUDeviceManager* LuxonEditor::EngineApplication::GetGPUApplication()
{
	return m_appInstance.m_gpuApplication;
}


LuxonEditor::EngineApplication::EngineApplication(const ApplicationConfig& config)
	:m_projectPath(config.projectPath),
	m_assetWatcher(new AssetDirectoryWatcher(config.projectPath + "/Assets")),
	m_graphicAPI(config.graphicAPI)
{
	m_assetManager = new AssetRegistry(config.projectPath, m_assetWatcher);
}

LuxonEditor::EngineApplication::EngineApplication(EngineApplication&& src)
	:m_projectPath(src.m_projectPath), m_assetManager(src.m_assetManager),
	m_assetWatcher(src.m_assetWatcher), m_gpuApplication(src.m_gpuApplication),
	m_shaderRegistery(src.m_shaderRegistery), m_graphicAPI(src.m_graphicAPI)
{
	src.m_assetManager = nullptr;
	src.m_assetWatcher = nullptr;
	src.m_gpuApplication = nullptr;
	src.m_shaderRegistery = nullptr;
}

LuxonEditor::EngineApplication& LuxonEditor::EngineApplication::operator=(EngineApplication&& src)
{
	m_projectPath = std::string(std::move(src.m_projectPath));
	m_graphicAPI = src.m_graphicAPI;
	m_assetManager = src.m_assetManager;
	m_assetWatcher = src.m_assetWatcher;
	m_gpuApplication = src.m_gpuApplication;
	m_shaderRegistery = src.m_shaderRegistery;
	src.m_assetManager = nullptr;
	src.m_assetWatcher = nullptr;
	src.m_gpuApplication = nullptr;
	src.m_shaderRegistery = nullptr;
	return *this;
}

bool LuxonEditor::EngineApplication::Initialize(std::string& error)
{
	m_assetWatcher->Start();
	m_gpuApplication = CreateGPUApplication(m_graphicAPI);
	auto compiler = m_gpuApplication->CreateShaderRegistery();
	m_shaderRegistery = new EngineShaderRegistry(compiler.get(), m_assetWatcher);
	m_shaderRegistery->CompileAllShaders();
	m_selectionManager = new SelectionManager();
	m_assetManager->ImportAllAssets();
	return true;
}

void LuxonEditor::EngineApplication::ShutDown()
{
	if (m_shaderRegistery != nullptr)
		delete m_shaderRegistery;
	if(m_assetWatcher != nullptr)
		delete m_assetWatcher;
	if (m_assetManager != nullptr)
		delete m_assetManager;
	if (m_gpuApplication != nullptr)
		delete m_gpuApplication;
}
