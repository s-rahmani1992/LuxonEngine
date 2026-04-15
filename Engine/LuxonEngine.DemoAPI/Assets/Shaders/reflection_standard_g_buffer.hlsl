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
    float2 normPos : POSITION;
};

OBJECT_TRANSFORM_VAR(b0)

CAMERA_VAR(b1)

LIGHT_VAR(b2)

CONSTANT_VARIABLES_BEGIN
    float ambient;
    float diffuse;
    float specular;
CONSTANT_VARIABLES_END(constantVars, b3)

#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular

TEXTURE(_OutputTexture, float4, t0)

TEXTURE(_PositionTexture, float4, t1)

TEXTURE(_NormalTexture, float4, t2)

TEXTURE(mainTexture, float4, t3)

TEXTURE(reflectTexture, float4, t4)

SAMPLER(mainSampler, s0);

VS_OUTPUT vs_main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    float4 position = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vsOut.pos = position;
    vsOut.texCoord = vertexIn.texCoord;
    float2 ndc = position.xy;
    vsOut.normPos = float2(ndc.x, -ndc.y) * 0.5f + 0.5f;
    return vsOut;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET
{
    float2 ndc = input.pos.xy / float2(1280, 720);
    
    float2 uv = float2(ndc.x, ndc.y);
    float3 pos = _PositionTexture.SampleLevel(mainSampler, uv, 0).xyz;
    float3 norm = 2 * _NormalTexture.SampleLevel(mainSampler, uv, 0).xyz - 1;
    float4 reflectionData = _OutputTexture.Sample(mainSampler, uv, 0);
    
    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, pos, norm, ads);
    
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    
    float reflectivity = reflectTexture.Sample(mainSampler, input.texCoord).r;
    return float4(lightFactor * ((1 - reflectivity) * texColor.xyz + reflectivity * reflectionData.xyz), 1);
}