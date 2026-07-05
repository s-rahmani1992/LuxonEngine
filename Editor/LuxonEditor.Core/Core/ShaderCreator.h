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
	
	private:
		static std::string s_rayTracingCodeBegin;
	};
}