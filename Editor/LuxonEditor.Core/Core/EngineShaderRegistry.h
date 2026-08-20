#pragma once
#include <boost/uuid/uuid.hpp>
#include <map>
#include <filesystem>
#include <functional>
#include "GuidUtilities.h"

namespace LuxonEngine {
	namespace Rendering {
		struct ShaderCompileProperties;
		class ShaderRegistery;
		class ShaderProgram;
	}
	class SerializationStream;
}

namespace fs = std::filesystem;
namespace Render = LuxonEngine::Rendering;

namespace LuxonEditor {
	class AssetDirectoryWatcher;
	struct FileChangeEvent;

	using GUID = boost::uuids::uuid;

	struct ShaderEntry {
		GUID guid;
		std::string name;
		std::string compileError;
		LuxonEngine::Rendering::ShaderProgram* program;
	};
	
	class __declspec(dllexport) EngineShaderRegistry {
	public:
		using ShaderProgramChangedCallback = std::function<void(ShaderEntry*)>;
		using ShaderProgramDeletedCallback = std::function<void(ShaderEntry*)>;

		EngineShaderRegistry(Render::ShaderRegistery* shaderCompiler, AssetDirectoryWatcher* assetWatcher);
		~EngineShaderRegistry();
		void CompileAllShaders();
		LuxonEngine::Rendering::ShaderProgram* GetProgram(GUID guid);
		ShaderEntry* GetShaderEntry(GUID guid);
		ShaderEntry* GetShaderEntry(const LuxonEngine::Rendering::ShaderProgram* program);
		std::vector<ShaderEntry*> GetAllShaderEntries() const;
		static void FillProperties(LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
		static void SerializeProperties(const LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
		ShaderEntry* GetFalllbackShaderProgram(){
			auto entry = m_registeredPrograms.find(GuidGenerator::GenerateGUIDFromString("2e1bfe23-51b1-42e6-ad60-bf6c351e25f8"));
			return &entry->second; }
	
		size_t RegisterShaderChangedCallback(ShaderProgramChangedCallback callback);
		void UnregisterShaderChangedCallback(size_t callbackId);

		size_t RegisterShaderDeletedCallback(ShaderProgramDeletedCallback callback);
		void UnregisterShaderDeletedCallback(size_t callbackId);
		
	private:
		void OnAssetChanged(const FileChangeEvent& paths);
		void CompileAtPath(const fs::path& filePath, bool fireEvent = false);
		void InvokeShaderChangedCallback(ShaderEntry*);
		void InvokeShaderDeletedCallback(ShaderEntry*);
		std::map<GUID, ShaderEntry> m_registeredPrograms;
		size_t m_callbackID;
		Render::ShaderRegistery* m_shaderCompiler;
		AssetDirectoryWatcher* m_assetWatcher;

		std::map<size_t, ShaderProgramChangedCallback> m_programChangedCallbacks;
		size_t m_lastProgramChangedCallbackId = 0;

		std::map<size_t, ShaderProgramDeletedCallback> m_programDeletedCallbacks;
		size_t m_lastProgramDeletedCallbackId = 0;
	};
}