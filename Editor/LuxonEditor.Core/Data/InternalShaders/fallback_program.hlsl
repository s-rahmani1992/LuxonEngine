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
};

OBJECT_TRANSFORM_VAR(b1) 

CAMERA_VAR(b2) 

VS_OUTPUT vs_main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    return vsOut;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 1.0f, 1.0f);
}