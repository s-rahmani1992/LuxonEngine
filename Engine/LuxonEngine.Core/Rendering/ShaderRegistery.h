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

	struct ShaderCompileProperties {
		ShaderProgramType type;
		std::string model;
		union {
			RasterizationProgramProperties rasterProperties;
			RayTracingProgramProperties rayTracingProperties;
			ComputeProgramProperties computeProperties;
		};
		std::wstring folderPath;
	};

	class ShaderRegistery {
	public:

		/// <summary>
		/// abstract method for compiling file into a complete shader program
		/// </summary>
		/// <param name="fileName">name of the file</param>
		/// <param name="error">contains error message if compilation fails</param>
		/// <returns></returns>
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) = 0;
		
		/// <summary>
		/// abstract method for compiling file into a complete shader program
		/// </summary>
		/// <param name="fileName">name of the file</param>
		/// <param name="error">contains error message if compilation fails</param>
		/// <returns></returns>
		virtual ShaderProgram* CompileProgram(const Byte* shaderCode, const UInt64 codeLength, const ShaderCompileProperties& properties, std::string& error) = 0;

		/// <summary>
		/// Registers shader program with name in order to be retrieved later. used for internal shaders
		/// </summary>
		/// <param name="name"></param>
		/// <param name="program"></param>
		/// <param name="isRT"></param>
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) = 0;
	
		/// <summary>
		/// Gets Shader Program By GUID. returns null if the guid does not exist
		/// </summary>
		/// <param name="guid">id of the program</param>
		/// <returns></returns>
		virtual ref<ShaderProgram> GetProgramByGUID(boost::uuids::uuid guid) = 0;
	};
}
