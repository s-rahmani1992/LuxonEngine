#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"
#include "Common/RTStructs.hlsli"

OBJECT_TRANSFORM_VAR(b0)

CAMERA_VAR(b1)

TEXTURE(mainTexture, float4, t0)

SAMPLER(mainSampler, s0)

RT_OBJECT_INDEX_BUFFER_VAR(t1)

RT_OBJECT_VERTEX_BUFFER_VAR(t2)

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    
    payload.color = texColor.xyz;
    
    if(payload.targetMode == 1)
        payload.hit = 1;
    else if (payload.targetMode == 2)
        payload.hit = 0;
}