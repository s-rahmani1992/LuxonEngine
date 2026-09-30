#pragma once
#include <string>

namespace LuxonEngine {
	namespace Rendering {
		struct ShaderCompileProperties;
	}
	class SerializationStream;
}

namespace LuxonEditor {
	class ShaderCreator {
	public:
		static void CreateShader(const LuxonEngine::Rendering::ShaderCompileProperties& properties, const std::string& shaderName);
		static void CreateMeshShader(const LuxonEngine::Rendering::ShaderCompileProperties& properties, const std::string& shaderName, const char* meshMain, const char* pixelMain, const char* taskMain = nullptr);

	private:
		static void WriteShaderFiles(const LuxonEngine::Rendering::ShaderCompileProperties& properties, const std::string& shaderName, const std::string& shaderCode);

		static std::string s_rayTracingCodeBegin;
	};
}