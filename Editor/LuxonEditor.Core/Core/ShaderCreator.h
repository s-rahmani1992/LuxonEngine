#pragma once
#include <string>
#include <Rendering/ShaderCompiler.h>

namespace LuxonEngine {
	class SerializationStream;
}

namespace LuxonEditor {
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

	struct MeshProgramProperties {
		char* taskMain;
		char* meshMain;
		char* pixelMain;
	};

	struct ShaderCreationProperties : LuxonEngine::Rendering::ShaderCompileProperties {
		std::string fileName;
		union {
			RasterizationProgramProperties rasterProperties;
			RayTracingProgramProperties rayTracingProperties;
			ComputeProgramProperties computeProperties;
			MeshProgramProperties meshProperties;
		};
	};

	class ShaderCreator {
	public:
		static void CreateShader(const ShaderCreationProperties& properties);

	private:
		static void WriteShaderFiles(const ShaderCreationProperties& properties, const std::string& shaderCode);

		static std::string s_rayTracingCodeBegin;
	};
}