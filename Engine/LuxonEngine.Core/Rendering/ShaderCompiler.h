#pragma once
#include "../BasicTypes.h"
#include <string>
#include <boost/uuid/uuid.hpp>

namespace LuxonEngine::Rendering {
	enum class ShaderProgramType;
	class ShaderProgram;
	class Shader;

	struct RasterizationProgramProperties {
		char* vertexMain;
		char* pixelMain;
		char* geometryMain;
	};

	struct RayTracingProgramProperties {
		char* rayGen;
		char* miss;
		char* intersection;
		char* anyHit;
		char* closestHit;
	};

	struct ComputeProgramProperties {
		char* computeMain;
	};

	enum class ShaderUsage {
		User,	
		Internal
	};

	struct ShaderCompileProperties {
		ShaderProgramType type;
		ShaderUsage usage = ShaderUsage::User;
		std::string identifier;
		std::string name;
		std::string model;
		union {
			RasterizationProgramProperties rasterProperties;
			RayTracingProgramProperties rayTracingProperties;
			ComputeProgramProperties computeProperties;
		};
		std::wstring folderPath;
	};

	class ShaderCompiler {
	public:
		/// <summary>
		/// abstract method for compiling file into a complete shader program
		/// </summary>
		/// <param name="fileName">name of the file</param>
		/// <param name="error">contains error message if compilation fails</param>
		/// <returns></returns>
		virtual ShaderProgram* CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& properties, std::string& error) = 0;
	};
}
