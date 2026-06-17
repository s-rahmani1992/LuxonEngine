#pragma 
#include <boost/uuid/uuid.hpp>
#include <map>

namespace LuxonEngine {
	namespace Rendering {
		struct ShaderCompileProperties;
		class ShaderRegistery;
		class ShaderProgram;
	}
	class SerializationStream;
}

namespace Render = LuxonEngine::Rendering;

namespace LuxonEditor {
	class AssetDirectoryWatcher;
	struct FileChangeEvent;

	using GUID = boost::uuids::uuid;
	
	class EngineShaderRegistry {
	public:
		EngineShaderRegistry(Render::ShaderRegistery* shaderCompiler, AssetDirectoryWatcher* assetWatcher);
		~EngineShaderRegistry();
		void CompileAllShaders();
	private:
		void OnAssetChanged(const FileChangeEvent& paths);
		static void FillProperties(LuxonEngine::Rendering::ShaderCompileProperties& properties, LuxonEngine::SerializationStream& stream);
		std::map<GUID, LuxonEngine::Rendering::ShaderProgram*> m_registeredPrograms;
		size_t m_callbackID;
		Render::ShaderRegistery* m_shaderCompiler;
		AssetDirectoryWatcher* m_assetWatcher;
	};
}