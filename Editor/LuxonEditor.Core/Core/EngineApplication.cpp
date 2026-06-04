#include "EngineApplication.h"
#include "AssetRegistry.h"

LuxonEditor::EngineApplication LuxonEditor::EngineApplication::m_appInstance;

LuxonEditor::EngineApplication* LuxonEditor::EngineApplication::CreateApplication(const ApplicationConfig& config, std::string& error)
{
	m_appInstance = EngineApplication(config);

	if (m_appInstance.Initialize(error) == false)
		return nullptr;

    return &m_appInstance;
}


LuxonEditor::EngineApplication::EngineApplication(const ApplicationConfig& config)
	:m_projectPath(config.projectPath), m_assetManager(new AssetRegistry(config.projectPath))
{
}

bool LuxonEditor::EngineApplication::Initialize(std::string& error)
{
    return true;
}
