#pragma once
#include <boost/uuid/uuid.hpp>
#include <map>
#include <filesystem>

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
		EngineShaderRegistry(Render::ShaderRegistery* shaderCompiler, AssetDirectoryWatcher* assetWatcher);
		~EngineShaderRegistry();
		void CompileAllShaders();
		LuxonEngine::Rendering::ShaderProgram* GetProgram(GUID guid);
		ShaderEntry* GetShaderEntry(GUID guid);
		ShaderEntry* GetShaderEntry(const LuxonEngine::Rendering::ShaderProgram* program);
		std::vector<ShaderEntry*> GetAllShaderEntries() const;
		static void FillProperties(LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
		static void SerializeProperties(const LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
	
	private:
		void OnAssetChanged(const FileChangeEvent& paths);
		void CompileAtPath(const fs::path& filePath);
		std::map<GUID, ShaderEntry> m_registeredPrograms;
		size_t m_callbackID;
		Render::ShaderRegistery* m_shaderCompiler;
		AssetDirectoryWatcher* m_assetWatcher;
	};
}