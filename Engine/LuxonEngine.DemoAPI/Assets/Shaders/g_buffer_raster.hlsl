#include "Common/TransformStructs.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
};

struct PSOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    uint mask : SV_Target2;
};

OBJECT_TRANSFORM_VAR(b0)

CAMERA_VAR(b1)

VS_OUTPUT vs_main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vsOut.normal = mul(float4(vertexIn.norm, 1.0f), transformData.rotationMatrix).xyz;
    vsOut.worldPos = mul(float4(vertexIn.pos, 1.0f), transformData.modelMatrix).xyz;
    return vsOut;
}

PSOutput ps_main(VS_OUTPUT input)
{
    PSOutput psOut;
    psOut.position = float4(input.worldPos, 1.0f);
    psOut.normal = float4(normalize(input.normal) * 0.5f + 0.5f, 1.0f); // Encode normal to [0,1] range
    psOut.mask = 1; // Example mask value, can be modified as needed
    
    return psOut;
}