#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
    float3 worldPos : POSITION;
};

OBJECT_TRANSFORM_VAR(b0) 

CAMERA_VAR(b1) 

LIGHT_VAR(b2)

CONSTANT_VARIABLES_BEGIN
    float ambient;
    float diffuse;
    float specular;
    float textureFactor;
CONSTANT_VARIABLES_END(constantVars, b3)

#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular

TEXTURE(mainTexture, float4, t0)

SAMPLER(mainSampler, s0)

VS_OUTPUT vs_main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vsOut.texCoord = vertexIn.texCoord;
    vsOut.norm = mul(float4(vertexIn.norm, 1.0f), transformData.rotationMatrix).xyz;
    vsOut.worldPos = mul(float4(vertexIn.pos, 1.0f), transformData.modelMatrix).xyz;
    return vsOut;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET
{
    float3 ads = float3(ambient, diffuse, specular);
  
    float3 lightFactor = PhongLight(lightData, cameraData.position, input.worldPos, input.norm, ads);
    
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    return float4(lightFactor * texColor.xyz, 1);
}