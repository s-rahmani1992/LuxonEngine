#pragma once
#include <boost/uuid/uuid.hpp>
#include <map>
#include <filesystem>
#include <functional>
#include "GuidUtilities.h"
#include "ShaderCreator.h"
#include <Rendering/ShaderRegistery.h>

namespace LuxonEngine {
	namespace Rendering {
		class ShaderCompiler;
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
		std::string identifier;	// empty for user shaders
	};
	
	class __declspec(dllexport) EngineShaderRegistry : public Render::ShaderRegistery {
	public:
		using ShaderProgramChangedCallback = std::function<void(ShaderEntry*)>;
		using ShaderProgramDeletedCallback = std::function<void(ShaderEntry*)>;

		EngineShaderRegistry(Render::ShaderCompiler* shaderCompiler, AssetDirectoryWatcher* assetWatcher);
		~EngineShaderRegistry();
		void CompileAllShaders();
		LuxonEngine::Rendering::ShaderProgram* GetProgram(GUID guid);
		ShaderEntry* GetShaderEntry(GUID guid);
		ShaderEntry* GetShaderEntry(const LuxonEngine::Rendering::ShaderProgram* program);
		std::vector<ShaderEntry*> GetAllShaderEntries() const;
		static void FillProperties(LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
		static void SerializeProperties(const LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
		ShaderEntry* GetFalllbackShaderProgram(){
			return GetShaderEntry(GuidGenerator::GenerateGUIDFromString("2e1bfe23-51b1-42e6-ad60-bf6c351e25f8")); }
	
		size_t RegisterShaderChangedCallback(ShaderProgramChangedCallback callback);
		void UnregisterShaderChangedCallback(size_t callbackId);

		size_t RegisterShaderDeletedCallback(ShaderProgramDeletedCallback callback);
		void UnregisterShaderDeletedCallback(size_t callbackId);
		
	private:
		void OnAssetChanged(const FileChangeEvent& paths);
		void CompileAtPath(const fs::path& filePath, bool fireEvent = false);
		void InvokeShaderChangedCallback(ShaderEntry*);
		void InvokeShaderDeletedCallback(ShaderEntry*);
		void RemoveShaderProgram(const ShaderEntry& entry);
		std::map<GUID, ShaderEntry> m_registeredPrograms;	// user shaders
		std::map<GUID, ShaderEntry> m_internalPrograms;		// internal shaders, also registered by identifier
		size_t m_callbackID;
		AssetDirectoryWatcher* m_assetWatcher;

		std::map<size_t, ShaderProgramChangedCallback> m_programChangedCallbacks;
		size_t m_lastProgramChangedCallbackId = 0;

		std::map<size_t, ShaderProgramDeletedCallback> m_programDeletedCallbacks;
		size_t m_lastProgramDeletedCallbackId = 0;
	};
}