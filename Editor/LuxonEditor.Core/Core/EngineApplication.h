#pragma once
#include <string>

namespace LuxonEditor {
	class AssetRegistry;

	struct ApplicationConfig {
		std::string projectPath;
	};

	class EngineApplication {
	public:
		static EngineApplication* CreateApplication(const ApplicationConfig& config, std::string& error);
		static std::string& GetProjectPath() { return m_appInstance.m_projectPath; }
		static AssetRegistry* GetAssetManager() { return m_appInstance.m_assetManager; }
		
	private:
		static EngineApplication m_appInstance;

	public:
		EngineApplication() = default;
		bool Initialize(std::string& error);
	private:
		EngineApplication(const ApplicationConfig& config);

		std::string m_projectPath;

		AssetRegistry* m_assetManager;
	};
}