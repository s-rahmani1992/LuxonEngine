#pragma once
#include <string>

enum Graphic_API;

namespace LuxonEngine::Rendering {
	class GPUDeviceManager;
}

namespace LuxonEditor {
	class AssetRegistry;
	class AssetDirectoryWatcher;
	class EngineShaderRegistry;
	class SelectionManager;

	struct ApplicationConfig {
		std::string projectPath;
		Graphic_API graphicAPI;
	};

	class EngineApplication {
	public:
		static EngineApplication* CreateApplication(const ApplicationConfig& config, std::string& error);
		static std::string& GetProjectPath() { return m_appInstance.m_projectPath; }
		static AssetRegistry* GetAssetManager() { return m_appInstance.m_assetManager; }
		static SelectionManager* GetSelectionManager();
	private:
		static EngineApplication m_appInstance;

	public:
		EngineApplication() = default;
		EngineApplication(const EngineApplication&) = delete;
		EngineApplication& operator=(const EngineApplication&) = delete;
		EngineApplication(EngineApplication&& src);
		EngineApplication& operator=(EngineApplication&& src);
		EngineApplication(const ApplicationConfig& config);
		~EngineApplication() { 
			ShutDown(); 
		}
		bool Initialize(std::string& error);
		void ShutDown();
	private:
		Graphic_API m_graphicAPI;
		std::string m_projectPath;
		EngineShaderRegistry* m_shaderRegistery;
		AssetRegistry* m_assetManager;
		AssetDirectoryWatcher* m_assetWatcher;
		LuxonEngine::Rendering::GPUDeviceManager* m_gpuApplication = nullptr;
		SelectionManager* m_selectionManager = nullptr;
	};
}