#include "ShaderCreator.h"
#include <EngineAPI.h>
#include "GuidUtilities.h"
#include "EngineApplication.h"
#include <fstream>
#include <Core/SerializationStream.h>
#include "EngineShaderRegistry.h"

#define RAY_GENERATION_Code std::string(R"(
[shader("raygeneration")]
void $(RAY_GENERATION)()
{
    float3 pixelPos = CalculateScreenPosition(cameraData.inverseProjectionMatrix);
    
    RayDesc ray;
    ray.Origin = cameraData.position;
    ray.Direction = normalize(pixelPos - ray.Origin);
    ray.TMin = 0;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 0;
    TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, _missIndex, 0, ray, payLoad);
    
    uint3 launchIndex = DispatchRaysIndex();
    _OutputTexture[launchIndex.xy] = float4(payLoad.color, 1);
}

)")

#define CLOSEST_HIT_Code std::string(R"(
[shader("closesthit")]
void $(CLOSEST_HIT)(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = hitColor.xyz;
}

)")

#define MISS_Code std::string(R"(
[shader("miss")]
void $(MISS)(inout GeneralPayload payload)
{
    payload.color = missColor.xyz;
}

)") 

std::string LuxonEditor::ShaderCreator::s_rayTracingCodeBegin = R"(
#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

CONSTANT_VARIABLES_BEGIN
    float4 missColor;
    float4 hitColor;
    uint _missIndex;
CONSTANT_VARIABLES_END(constantVars, b0)
    
#define missColor constantVars.missColor
#define hitColor constantVars.hitColor
#define _missIndex constantVars._missIndex

CAMERA_VAR(b1)
    
RT_SCENE_VAR(t0)

RT_OUT_TEXTURE_VAR(u0)

)";

void LuxonEditor::ShaderCreator::CreateShader(const ShaderCreationProperties& properties)
{
	std::string templatePath = EngineApplication::GetProjectPath() + "/Data/ShaderTemplates/";
	std::string shaderCode;

	if (properties.type == LuxonEngine::Rendering::ShaderProgramType::Rasterization)
	{
		// load templete file into a string variable
		std::ifstream templateFile(templatePath + (properties.rasterProperties.geometryMain == nullptr ? "raster_simple_template.hlsl" : "raster_geometry_template.hlsl"));
		std::string templateContent((std::istreambuf_iterator<char>(templateFile)),
			std::istreambuf_iterator<char>());

		shaderCode = templateContent.replace(templateContent.find("$(VERTEX_MAIN)"), std::string("$(VERTEX_MAIN)").length(), properties.rasterProperties.vertexMain);
		shaderCode = shaderCode.replace(shaderCode.find("$(PIXEL_MAIN)"), std::string("$(PIXEL_MAIN)").length(), properties.rasterProperties.pixelMain);

		if (properties.rasterProperties.geometryMain != nullptr)
		{
			shaderCode = shaderCode.replace(shaderCode.find("$(GEOMETRY_MAIN)"), std::string("$(GEOMETRY_MAIN)").length(), properties.rasterProperties.geometryMain);
		}
	}
	else if (properties.type == LuxonEngine::Rendering::ShaderProgramType::RayTracing)
	{
		// load templete file into a string variable
		std::ifstream templateFile(templatePath + "raytracing_template.hlsl");
		std::string templateContent((std::istreambuf_iterator<char>(templateFile)),
			std::istreambuf_iterator<char>());

		shaderCode = s_rayTracingCodeBegin;
		std::string shaderStageCode;

		if (properties.rayTracingProperties.rayGen != nullptr)
		{
			shaderStageCode = RAY_GENERATION_Code;
			shaderCode += shaderStageCode.replace(shaderStageCode.find("$(RAY_GENERATION)"), std::string("$(RAY_GENERATION)").length(), properties.rayTracingProperties.rayGen);
		}

		if (properties.rayTracingProperties.closestHit != nullptr)
		{
			shaderStageCode = CLOSEST_HIT_Code;
			shaderCode += shaderStageCode.replace(shaderStageCode.find("$(CLOSEST_HIT)"), std::string("$(CLOSEST_HIT)").length(), properties.rayTracingProperties.closestHit);
		}

		if (properties.rayTracingProperties.miss != nullptr)
		{
			shaderStageCode = MISS_Code;
			shaderCode += shaderStageCode.replace(shaderStageCode.find("$(MISS)"), std::string("$(MISS)").length(), properties.rayTracingProperties.miss);
		}
	}
	else if (properties.type == LuxonEngine::Rendering::ShaderProgramType::Compute)
	{
		// load templete file into a string variable
		std::ifstream templateFile(templatePath + "compute_template.hlsl");
		std::string templateContent((std::istreambuf_iterator<char>(templateFile)),
			std::istreambuf_iterator<char>());

		shaderCode = templateContent.replace(templateContent.find("$(COMPUTE_MAIN)"), std::string("$(COMPUTE_MAIN)").length(), properties.computeProperties.computeMain);
	}
	else if (properties.type == LuxonEngine::Rendering::ShaderProgramType::Mesh)
	{
		const auto& mesh = properties.meshProperties;

		// load templete file into a string variable
		std::ifstream templateFile(templatePath + (mesh.taskMain == nullptr ? "mesh_simple_template.hlsl" : "mesh_task_template.hlsl"));
		std::string templateContent((std::istreambuf_iterator<char>(templateFile)),
			std::istreambuf_iterator<char>());

		shaderCode = templateContent.replace(templateContent.find("$(MESH_MAIN)"), std::string("$(MESH_MAIN)").length(), mesh.meshMain);
		shaderCode = shaderCode.replace(shaderCode.find("$(PIXEL_MAIN)"), std::string("$(PIXEL_MAIN)").length(), mesh.pixelMain);

		if (mesh.taskMain != nullptr)
		{
			shaderCode = shaderCode.replace(shaderCode.find("$(TASK_MAIN)"), std::string("$(TASK_MAIN)").length(), mesh.taskMain);
		}
	}
	else
		return;

	WriteShaderFiles(properties, shaderCode);
}

void LuxonEditor::ShaderCreator::WriteShaderFiles(const ShaderCreationProperties& properties, const std::string& shaderCode)
{
	std::string metaString;
	LuxonEngine::SerializationStream metadataStream, shaderMetaStream;
	metadataStream.SetGuid("uuid", LuxonEditor::GuidGenerator::GenerateGUID());

	ShaderCreationProperties metaProperties = properties;
	if (metaProperties.usage == LuxonEngine::Rendering::ShaderUsage::User && metaProperties.name.empty())
		metaProperties.name = properties.fileName;

	EngineShaderRegistry::SerializeProperties(metaProperties, shaderMetaStream);

	metadataStream.SetObject("data", shaderMetaStream);
	metaString = metadataStream.ToString();

	const std::filesystem::path outputFolder = properties.folderPath.empty() ? std::filesystem::path(EngineApplication::GetProjectPath() + "/Assets/Shaders/") : std::filesystem::path(properties.folderPath);
	std::string shaderPath = (outputFolder / (properties.fileName + ".hlsl")).string();
	std::string shaderMetaPath = shaderPath + ".json";

	metadataStream.SaveToFile(shaderMetaPath);

	// write the shader code to a file
	std::ofstream shaderFile(shaderPath);
	shaderFile << shaderCode;
	shaderFile.close();
}
